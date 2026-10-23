import Foundation
import Testing
@testable import KazCore

let resources = URL(fileURLWithPath: #filePath)
    .deletingLastPathComponent().deletingLastPathComponent().deletingLastPathComponent()
    .appendingPathComponent("Resources")
let haveModel = FileManager.default.fileExists(atPath: resources.appendingPathComponent("vocab.json").path)

let vocabJSON = """
{"maxlen": 8, "num_tags": 3,
 "vocab": ["<pad>", "<unk>", "<digit>", "<latin>", "а", "к", "з", "с", "т", "н", " "],
 "special": {"pad": 0, "unk": 1, "digit": 2, "latin": 3},
 "rules": {"к": ["к", "қ"], "а": ["а", "ә"], "н": ["н", "ң"]},
 "to_russian": {"қ": "к", "ә": "а", "ң": "н"}}
"""

/// Always prefers the second variant for к, the first for everything else.
struct FakeModel: TagModel {
    let vocab: Vocab
    func probabilities(_ ids: [Int32]) throws -> [[Float]] {
        let k = Int32(vocab.vocab.firstIndex(of: "к")!)
        return ids.map { $0 == k ? [0.2, 0.8, 0] : [0.9, 0.1, 0] }
    }
}

func fakeConverter() throws -> Converter {
    let vocab = try JSONDecoder().decode(Vocab.self, from: Data(vocabJSON.utf8))
    return Converter(vocab: vocab, model: FakeModel(vocab: vocab), userDict: UserDict(url: nil))
}

@Test func russianizeMapsKazakhLetters() throws {
    let c = try fakeConverter()
    #expect(c.vocab.russianize("ҚАЗАҚ ӘН") == "казак ан")
}

@Test func encodeHandlesDigitsLatinAndUnknown() throws {
    let v = try fakeConverter().vocab
    #expect(v.encode("к7z!") == [5, 2, 3, 1])
}

@Test func beamRanksByProbability() throws {
    let r = try fakeConverter().convert("казак")
    #expect(r.candidates.first == "қазақ")
    #expect(r.modelTop == "қазақ")
    #expect(r.candidates.contains("казак"))  // typed word always offered
}

@Test func preservesCase() throws {
    let r = try fakeConverter().convert("Казак")
    #expect(r.candidates.first == "Қазақ")
}

@Test func userChoiceWinsAndIsForgotten() throws {
    let c = try fakeConverter()
    c.learn(typed: "кан", chosen: "кан", modelTop: "қан")
    #expect(c.convert("кан").candidates.first == "кан")
    #expect(c.convert("Кан").candidates.first == "Кан")
    // Picking the model's choice again removes the override.
    c.learn(typed: "кан", chosen: "қан", modelTop: "қан")
    #expect(c.userDict.lookup("кан") == nil)
}

@Test func userDictRoundTrip() throws {
    let url = FileManager.default.temporaryDirectory
        .appendingPathComponent(UUID().uuidString).appendingPathComponent("d.json")
    UserDict(url: url).set("кол", "қол")
    #expect(UserDict(url: url).lookup("кол") == "қол")
}

@Suite(.enabled(if: haveModel, "run `uv run python -m ml.export` first"))
struct RealModelTests {
    let converter: Converter

    init() throws {
        converter = try Converter(resources: resources, userDict: UserDict(url: nil))
    }

    @Test(arguments: [
        ("казакстан", "қазақстан"),
        ("кулки", "күлкі"),
        ("рахмет", "рахмет"),
        ("бугин", "бүгін"),
        ("менин", "менің"),
    ])
    func commonWords(typed: String, want: String) {
        #expect(converter.convert(typed).candidates.first == want)
    }

    @Test func contextChangesAnswer() {
        // "қол" (hand) vs "көл" (lake): both should at least be offered.
        let r = converter.convert("кол", context: "сол ")
        #expect(r.candidates.contains("қол"))
        #expect(r.candidates.contains("көл"))
    }

    @Test func latencyUnder10ms() {
        _ = converter.convert("казакстан")
        let start = Date()
        for _ in 0..<50 { _ = converter.convert("казакстан", context: "біз тұратын ел ") }
        let ms = Date().timeIntervalSince(start) / 50 * 1000
        print("latency \(ms) ms per conversion")
        // Shared CI runners are too slow and noisy for the target; they only report it.
        if ProcessInfo.processInfo.environment["CI"] == nil { #expect(ms < 10) }
    }
}
