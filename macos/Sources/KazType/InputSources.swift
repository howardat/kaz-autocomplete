import Carbon
import Foundation

/// Text Input Sources helpers: registration and the Russian layout used for keys.
enum InputSources {
    static let fallbackLayout = "com.apple.keylayout.RussianWin"
    private static let savedLayoutKey = "russianLayoutID"

    static func register(bundleURL: URL) -> Bool {
        let status = TISRegisterInputSource(bundleURL as CFURL)
        guard status == noErr else {
            print("TISRegisterInputSource failed: \(status)")
            return false
        }
        let bundleID = Bundle.main.bundleIdentifier!
        let filter = [kTISPropertyBundleID as String: bundleID] as CFDictionary
        let sources = TISCreateInputSourceList(filter, true)?.takeRetainedValue() as? [TISInputSource] ?? []
        for src in sources where string(src, kTISPropertyInputSourceType) == (kTISTypeKeyboardInputMode as String) {
            TISEnableInputSource(src)
            print("enabled \(string(src, kTISPropertyInputSourceID) ?? "?")")
        }
        return true
    }

    /// The Russian layout to type with: the one active before switching to us,
    /// else the last one seen, else any enabled Russian layout, else Russian - PC.
    static func russianLayout() -> String {
        let defaults = UserDefaults.standard
        if let current = TISCopyCurrentKeyboardLayoutInputSource()?.takeRetainedValue(),
           let id = string(current, kTISPropertyInputSourceID), isRussian(id) {
            defaults.set(id, forKey: savedLayoutKey)
            return id
        }
        if let saved = defaults.string(forKey: savedLayoutKey) { return saved }
        let filter = [kTISPropertyInputSourceType as String: kTISTypeKeyboardLayout as String] as CFDictionary
        let layouts = TISCreateInputSourceList(filter, false)?.takeRetainedValue() as? [TISInputSource] ?? []
        if let id = layouts.compactMap({ string($0, kTISPropertyInputSourceID) }).first(where: isRussian) {
            return id
        }
        return fallbackLayout
    }

    private static func isRussian(_ id: String) -> Bool { id.contains("Russian") }

    private static func string(_ src: TISInputSource, _ key: CFString) -> String? {
        guard let ptr = TISGetInputSourceProperty(src, key) else { return nil }
        return Unmanaged<CFString>.fromOpaque(ptr).takeUnretainedValue() as String
    }
}
