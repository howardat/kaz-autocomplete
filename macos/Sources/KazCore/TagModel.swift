import CoreML
import Foundation

/// Per-char tag probabilities for a right-aligned window of ids.
public protocol TagModel {
    /// ids.count == maxlen. Returns maxlen rows of numTags probabilities.
    func probabilities(_ ids: [Int32]) throws -> [[Float]]
}

public final class CoreMLTagModel: TagModel {
    private let model: MLModel
    private let maxlen: Int
    private let numTags: Int

    /// Loads a .mlpackage, compiling it on the fly (no Xcode needed at build time).
    public init(packageURL: URL, maxlen: Int, numTags: Int) throws {
        let compiled = try MLModel.compileModel(at: packageURL)
        let config = MLModelConfiguration()
        config.computeUnits = .cpuOnly  // lowest latency for a tiny model, no GPU wake-up
        model = try MLModel(contentsOf: compiled, configuration: config)
        try? FileManager.default.removeItem(at: compiled)
        self.maxlen = maxlen
        self.numTags = numTags
    }

    public func probabilities(_ ids: [Int32]) throws -> [[Float]] {
        let input = try MLMultiArray(shape: [1, NSNumber(value: maxlen)], dataType: .int32)
        for (i, id) in ids.enumerated() { input[i] = NSNumber(value: id) }
        let out = try model.prediction(from: MLDictionaryFeatureProvider(dictionary: ["chars": input]))
        guard let probs = out.featureValue(for: "probs")?.multiArrayValue else {
            throw NSError(domain: "KazType", code: 1, userInfo: [NSLocalizedDescriptionKey: "no probs output"])
        }
        return (0..<maxlen).map { t in
            (0..<numTags).map { k in probs[[0, NSNumber(value: t), NSNumber(value: k)]].floatValue }
        }
    }
}
