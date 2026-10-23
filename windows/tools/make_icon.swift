// Draws the language bar icon (a white "Қ" on a zinc-950 rounded square,
// shadcn's primary button colors) as a multi-size PNG-compressed .ico.
// Runs on macOS; the result is checked in as Resources/KazType.ico.
// Usage: swift tools/make_icon.swift Resources/KazType.ico
import AppKit

func png(_ px: Int) -> Data {
    let rep = NSBitmapImageRep(
        bitmapDataPlanes: nil, pixelsWide: px, pixelsHigh: px, bitsPerSample: 8,
        samplesPerPixel: 4, hasAlpha: true, isPlanar: false, colorSpaceName: .deviceRGB,
        bytesPerRow: 0, bitsPerPixel: 0)!
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
    let s = CGFloat(px)
    let inset = s * 0.03
    let rect = NSRect(x: inset, y: inset, width: s - 2 * inset, height: s - 2 * inset)
    NSColor(srgbRed: 9 / 255, green: 9 / 255, blue: 11 / 255, alpha: 1).setFill()
    NSBezierPath(roundedRect: rect, xRadius: s * 0.22, yRadius: s * 0.22).fill()
    let glyph = NSAttributedString(string: "Қ", attributes: [
        .font: NSFont.systemFont(ofSize: s * 0.68, weight: .semibold),
        .foregroundColor: NSColor(srgbRed: 250 / 255, green: 250 / 255, blue: 250 / 255, alpha: 1),
    ])
    let size = glyph.size()
    glyph.draw(at: NSPoint(x: (s - size.width) / 2 + s * 0.02, y: (s - size.height) / 2))
    NSGraphicsContext.restoreGraphicsState()
    return rep.representation(using: .png, properties: [:])!
}

let sizes = [16, 20, 24, 32, 40, 48, 64, 256]
let images = sizes.map(png)
var ico = Data()
func u16(_ v: Int) { ico.append(contentsOf: [UInt8(v & 0xFF), UInt8(v >> 8)]) }
func u32(_ v: Int) { u16(v & 0xFFFF); u16(v >> 16) }
u16(0); u16(1); u16(sizes.count)
var offset = 6 + 16 * sizes.count
for (px, data) in zip(sizes, images) {
    ico.append(UInt8(px == 256 ? 0 : px)); ico.append(UInt8(px == 256 ? 0 : px))
    ico.append(0); ico.append(0)
    u16(1); u16(32); u32(data.count); u32(offset)
    offset += data.count
}
images.forEach { ico.append($0) }
try! ico.write(to: URL(fileURLWithPath: CommandLine.arguments[1]))
