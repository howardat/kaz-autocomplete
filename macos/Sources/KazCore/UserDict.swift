import Foundation

/// Words the user picked over the model's top guess. Keyed by the russianized,
/// lowercased word; the value is the lowercased Kazakh word to put first.
public final class UserDict {
    private(set) public var entries: [String: String] = [:]
    private let url: URL?

    /// url nil keeps the dictionary in memory only (tests).
    public init(url: URL?) {
        self.url = url
        if let url, let data = try? Data(contentsOf: url),
           let saved = try? JSONDecoder().decode([String: String].self, from: data) {
            entries = saved
        }
    }

    public static var defaultURL: URL {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("KazType/user_dict.json")
    }

    public func lookup(_ key: String) -> String? { entries[key] }

    public func set(_ key: String, _ word: String?) {
        guard entries[key] != word else { return }
        entries[key] = word
        save()
    }

    private func save() {
        guard let url else { return }
        do {
            try FileManager.default.createDirectory(
                at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
            let enc = JSONEncoder()
            enc.outputFormatting = [.prettyPrinted, .sortedKeys]
            try enc.encode(entries).write(to: url, options: .atomic)
        } catch {
            NSLog("KazType: could not save user dict: \(error)")
        }
    }
}
