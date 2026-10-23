import Foundation

/// Turns a word typed with Russian letters into ranked Kazakh candidates.
public final class Converter {
    public struct Result: Equatable {
        /// Best first. Never empty; the typed word is the last resort.
        public let candidates: [String]
        /// What the model alone would pick, before the user dictionary.
        public let modelTop: String
    }

    public let vocab: Vocab
    private let model: TagModel
    public let userDict: UserDict

    public init(vocab: Vocab, model: TagModel, userDict: UserDict) {
        self.vocab = vocab
        self.model = model
        self.userDict = userDict
    }

    /// Loads vocab.json and KazTagger.mlpackage from a resources directory.
    public convenience init(resources: URL, userDict: UserDict) throws {
        let vocab = try Vocab.load(resources.appendingPathComponent("vocab.json"))
        let model = try CoreMLTagModel(
            packageURL: resources.appendingPathComponent("KazTagger.mlpackage"),
            maxlen: vocab.maxlen, numTags: vocab.numTags)
        self.init(vocab: vocab, model: model, userDict: userDict)
    }

    /// Key used by the user dictionary for a typed word.
    public func key(for typed: String) -> String { vocab.russianize(typed) }

    /// typed: the word as typed, any case. context: text before it, any script.
    public func convert(_ typed: String, context: String = "", limit: Int = 5) -> Result {
        let word = Array(vocab.russianize(typed))
        guard !word.isEmpty else { return Result(candidates: [typed], modelTop: typed) }

        var ids = vocab.encode(String(word))
        let ctx = vocab.encode(vocab.russianize(context))
        ids = Array((ctx + ids).suffix(vocab.maxlen))
        let wordIds = ids.suffix(word.count)
        let padded = Array(repeating: vocab.special.pad, count: vocab.maxlen - ids.count) + ids

        var ranked: [String]
        if let probs = try? model.probabilities(padded) {
            let rows = Array(probs.suffix(wordIds.count))
            let offset = word.count - rows.count  // only if the word is longer than maxlen
            ranked = beam(Array(word.suffix(rows.count)), rows, width: limit).map {
                String(word.prefix(offset)) + $0
            }
        } else {
            ranked = [String(word)]
        }
        let modelTop = ranked[0]

        if let mine = userDict.lookup(String(word)) {
            ranked.removeAll { $0 == mine }
            ranked.insert(mine, at: 0)
        }
        ranked = Array(ranked.prefix(limit))
        if !ranked.contains(String(word)) {
            // Keep the raw word reachable, in place of the weakest guess.
            if ranked.count >= limit { ranked.removeLast() }
            ranked.append(String(word))
        }
        let cased = ranked.map { matchCase($0, to: typed) }
        return Result(candidates: Array(cased), modelTop: matchCase(modelTop, to: typed))
    }

    /// Remember that `typed` should become `chosen` when it differs from the model.
    public func learn(typed: String, chosen: String, modelTop: String) {
        let k = key(for: typed)
        userDict.set(k, chosen.lowercased() == modelTop.lowercased() ? nil : chosen.lowercased())
    }

    private func beam(_ word: [Character], _ probs: [[Float]], width: Int) -> [String] {
        var beams: [(String, Float)] = [("", 0)]
        for (c, p) in zip(word, probs) {
            let vs = vocab.variants(of: c)
            if vs.count == 1 {
                beams = beams.map { ($0.0 + vs[0], $0.1) }
                continue
            }
            beams = beams.flatMap { b in
                vs.indices.map { j in (b.0 + vs[j], b.1 + log(max(p[j], 1e-9))) }
            }
            beams.sort { $0.1 > $1.1 }
            beams = Array(beams.prefix(width))
        }
        return beams.map(\.0)
    }

    /// Copy per-char case from what was typed.
    private func matchCase(_ word: String, to typed: String) -> String {
        String(zip(word, typed).map { w, t in
            t.isUppercase ? Character(String(w).uppercased()) : w
        }) + word.dropFirst(typed.count)
    }
}
