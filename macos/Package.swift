// swift-tools-version: 6.0
import PackageDescription

let package = Package(
    name: "KazType",
    platforms: [.macOS(.v13)],
    targets: [
        // Conversion logic, no UI. Unit tested.
        .target(name: "KazCore"),
        // The input method process. Bundled into KazType.app by the Makefile.
        .executableTarget(
            name: "KazType",
            dependencies: ["KazCore"],
            linkerSettings: [.linkedFramework("InputMethodKit"), .linkedFramework("Carbon")]
        ),
        .testTarget(name: "KazCoreTests", dependencies: ["KazCore"]),
    ],
    swiftLanguageModes: [.v5]
)
