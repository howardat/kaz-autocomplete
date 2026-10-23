import Cocoa
import InputMethodKit
import KazCore

/// One instance per client text field. Buffers the word being typed, shows it
/// converted as marked text, and commits on space, punctuation or a pick.
@objc(KazInputController)
final class KazInputController: IMKInputController {
    private var typed = ""
    private var context = ""
    private var result: Converter.Result?
    private var selected = 0

    private enum Key {
        static let backspace: UInt16 = 51, escape: UInt16 = 53
        static let returnKey: UInt16 = 36, enter: UInt16 = 76
        static let up: UInt16 = 126, down: UInt16 = 125
    }

    override func activateServer(_ sender: Any!) {
        super.activateServer(sender)
        (sender as? IMKTextInput)?.overrideKeyboard(withKeyboardNamed: InputSources.russianLayout())
    }

    override func deactivateServer(_ sender: Any!) {
        commit(sender as? IMKTextInput)
        super.deactivateServer(sender)
    }

    override func commitComposition(_ sender: Any!) {
        commit(sender as? IMKTextInput)
    }

    override func recognizedEvents(_ sender: Any!) -> Int {
        Int(NSEvent.EventTypeMask.keyDown.rawValue)
    }

    override func handle(_ event: NSEvent!, client sender: Any!) -> Bool {
        guard let event, event.type == .keyDown, let client = sender as? IMKTextInput else { return false }

        if !event.modifierFlags.intersection([.command, .control, .option]).isEmpty {
            commit(client)
            return false
        }

        let chars = event.characters ?? ""
        if let c = chars.first, chars.count == 1, isCyrillicLetter(c), App.converter != nil {
            if typed.isEmpty { context = readContext(client) }
            typed.append(c)
            update(client)
            return true
        }
        guard !typed.isEmpty else { return false }

        switch event.keyCode {
        case Key.backspace:
            typed.removeLast()
            typed.isEmpty ? reset(client, clearMarked: true) : update(client)
            return true
        case Key.escape:
            insert(typed, client)
            return true
        case Key.returnKey, Key.enter:
            commit(client)
            return true
        case Key.up, Key.down:
            moveSelection(event.keyCode == Key.down ? 1 : -1, client)
            return true
        default:
            break
        }
        if let d = chars.first?.wholeNumberValue, (1...9).contains(d),
           let r = result, r.candidates.count > 1, d <= r.candidates.count {
            selected = d - 1
            commit(client)
            return true
        }
        // Space, punctuation, anything else: commit, then let the app handle the key.
        commit(client)
        return false
    }

    // MARK: Candidate window

    override func candidates(_ sender: Any!) -> [Any]! {
        result?.candidates ?? []
    }

    override func candidateSelectionChanged(_ candidateString: NSAttributedString!) {
        guard let s = candidateString?.string, let i = result?.candidates.firstIndex(of: s) else { return }
        selected = i
        if let client = client() { setMarked(s, client) }
    }

    override func candidateSelected(_ candidateString: NSAttributedString!) {
        guard let s = candidateString?.string, let i = result?.candidates.firstIndex(of: s) else { return }
        selected = i
        commit(client())
    }

    // MARK: Composition

    private func update(_ client: IMKTextInput) {
        guard let converter = App.converter else { return }
        result = converter.convert(typed, context: context, limit: 9)
        selected = 0
        setMarked(result!.candidates[0], client)
        if result!.candidates.count > 1 {
            App.candidates.update()
            App.candidates.show(kIMKLocateCandidatesBelowHint)
        } else {
            App.candidates.hide()
        }
    }

    private func moveSelection(_ delta: Int, _ client: IMKTextInput) {
        guard let r = result, r.candidates.count > 1 else { return }
        selected = (selected + delta + r.candidates.count) % r.candidates.count
        setMarked(r.candidates[selected], client)
        App.candidates.selectCandidate(withIdentifier: App.candidates.candidateIdentifier(atLineNumber: selected))
    }

    private func commit(_ client: IMKTextInput?) {
        guard let client, !typed.isEmpty, let r = result else { return }
        let chosen = r.candidates[selected]
        App.converter?.learn(typed: typed, chosen: chosen, modelTop: r.modelTop)
        insert(chosen, client)
    }

    private func insert(_ text: String, _ client: IMKTextInput) {
        client.insertText(text, replacementRange: NSRange(location: NSNotFound, length: 0))
        reset(client)
    }

    /// clearMarked: remove the marked text too. Not needed after insertText,
    /// which replaces it.
    private func reset(_ client: IMKTextInput, clearMarked: Bool = false) {
        if clearMarked {
            client.setMarkedText("", selectionRange: NSRange(location: 0, length: 0),
                                 replacementRange: NSRange(location: NSNotFound, length: 0))
        }
        typed = ""
        result = nil
        selected = 0
        App.candidates.hide()
    }

    private func setMarked(_ text: String, _ client: IMKTextInput) {
        client.setMarkedText(text, selectionRange: NSRange(location: text.utf16.count, length: 0),
                             replacementRange: NSRange(location: NSNotFound, length: 0))
    }

    /// Up to 64 chars before the cursor, if the app exposes them.
    private func readContext(_ client: IMKTextInput) -> String {
        let sel = client.selectedRange()
        guard sel.location != NSNotFound, sel.location > 0 else { return "" }
        let start = max(0, sel.location - 64)
        let range = NSRange(location: start, length: sel.location - start)
        return client.attributedSubstring(from: range)?.string ?? ""
    }

    private func isCyrillicLetter(_ c: Character) -> Bool {
        c.isLetter && c.unicodeScalars.allSatisfy { (0x0400...0x04FF).contains($0.value) }
    }
}
