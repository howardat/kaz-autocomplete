import Foundation

/// Mirror of ml/rules.py, loaded from vocab.json written by ml/export.py.
public struct Vocab: Decodable, Sendable {
    public struct Special: Decodable, Sendable {
        let pad: Int32, unk: Int32, digit: Int32, latin: Int32
    }

    public let maxlen: Int
    public let numTags: Int
    public let vocab: [String]
    public let special: Special
    /// Russian-keyboard letter -> Kazakh variants; index is the tag.
    public let rules: [String: [String]]
    /// Kazakh-only letter -> Russian-keyboard letter (қ -> к, ...).
    public let toRussian: [String: String]

    private var ids: [Character: Int32] = [:]

    enum CodingKeys: String, CodingKey {
        case maxlen, numTags = "num_tags", vocab, special, rules, toRussian = "to_russian"
    }

    public init(from decoder: Decoder) throws {
        let c = try decoder.container(keyedBy: CodingKeys.self)
        maxlen = try c.decode(Int.self, forKey: .maxlen)
        numTags = try c.decode(Int.self, forKey: .numTags)
        vocab = try c.decode([String].self, forKey: .vocab)
        special = try c.decode(Special.self, forKey: .special)
        rules = try c.decode([String: [String]].self, forKey: .rules)
        toRussian = try c.decode([String: String].self, forKey: .toRussian)
        for (i, s) in vocab.enumerated() where s.count == 1 {
            ids[Character(s)] = Int32(i)
        }
    }

    public static func load(_ url: URL) throws -> Vocab {
        try JSONDecoder().decode(Vocab.self, from: Data(contentsOf: url))
    }

    /// Lowercase and map Kazakh letters back to what a Russian keyboard types.
    public func russianize(_ text: String) -> String {
        String(text.lowercased().map { c in toRussian[String(c)].map(Character.init) ?? c })
    }

    /// Ids for already-russianized text. Same rules as ml.rules.char_id.
    public func encode(_ text: String) -> [Int32] {
        text.map { c in
            if let id = ids[c] { return id }
            if c.isNumber { return special.digit }
            if c.isASCII && c.isLetter { return special.latin }
            if c.isWhitespace { return ids[" "] ?? special.unk }
            return special.unk
        }
    }

    /// Number of variants for a russianized letter (1 = never changes).
    public func variants(of c: Character) -> [String] {
        rules[String(c)] ?? [String(c)]
    }
}
