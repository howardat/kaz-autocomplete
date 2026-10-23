import Cocoa
import InputMethodKit
import KazCore

/// Process-wide objects shared by every input controller (one per client app).
enum App {
    nonisolated(unsafe) static var server: IMKServer!
    nonisolated(unsafe) static var candidates: IMKCandidates!
    nonisolated(unsafe) static var converter: Converter?
}

let info = Bundle.main.infoDictionary ?? [:]

// `KazType --register`: register and enable the input source (used by make install).
if CommandLine.arguments.contains("--register") {
    exit(InputSources.register(bundleURL: Bundle.main.bundleURL) ? 0 : 1)
}

App.server = IMKServer(
    name: info["InputMethodConnectionName"] as? String,
    bundleIdentifier: Bundle.main.bundleIdentifier)
App.candidates = IMKCandidates(
    server: App.server, panelType: kIMKSingleColumnScrollingCandidatePanel)

do {
    App.converter = try Converter(
        resources: Bundle.main.resourceURL!, userDict: UserDict(url: UserDict.defaultURL))
} catch {
    // Keep running: the controller passes letters through unconverted.
    NSLog("KazType: failed to load model: \(error)")
}

NSApplication.shared.run()
