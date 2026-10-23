# Build and install the KazType input method. Needs only Command Line Tools.
#   make install   build, sign, copy to ~/Library/Input Methods, register
#   make test      unit tests for KazCore
#   make pkg       installer for /Library/Input Methods -> build/KazType-<version>.pkg

APP      := build/KazType.app
DEST     := $(HOME)/Library/Input Methods
VERSION  := $(shell /usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" Info.plist)
PKG      := build/KazType-$(VERSION).pkg

.PHONY: all app test install uninstall pkg clean

all: app

# Universal: one build per architecture, then lipo (multi-arch builds need Xcode).
app:
	swift build -c release --product KazType --arch arm64
	swift build -c release --product KazType --arch x86_64
	@test -f Resources/vocab.json || (echo "Missing model. Run: uv run python -m ml.export" && exit 1)
	rm -rf $(APP)
	mkdir -p $(APP)/Contents/MacOS $(APP)/Contents/Resources
	lipo -create -output $(APP)/Contents/MacOS/KazType \
		.build/arm64-apple-macosx/release/KazType .build/x86_64-apple-macosx/release/KazType
	cp Info.plist $(APP)/Contents/Info.plist
	cp -R Resources/ $(APP)/Contents/Resources/
	swift tools/make_icon.swift $(APP)/Contents/Resources/icon.tiff
	codesign --force --sign - $(APP)

test:
	swift test

install: app
	-pkill -x KazType
	rm -rf "$(DEST)/KazType.app"
	mkdir -p "$(DEST)"
	cp -R $(APP) "$(DEST)/"
	"$(DEST)/KazType.app/Contents/MacOS/KazType" --register
	@echo "Installed. If KazType is missing from Input Sources, log out and back in."

uninstall:
	-pkill -x KazType
	rm -rf "$(DEST)/KazType.app"

# Not relocatable: otherwise Installer updates any other KazType.app it finds instead.
pkg: app
	rm -rf build/pkgroot && mkdir -p build/pkgroot
	cp -R $(APP) build/pkgroot/
	pkgbuild --analyze --root build/pkgroot build/component.plist
	plutil -replace 0.BundleIsRelocatable -bool NO build/component.plist
	pkgbuild --root build/pkgroot --component-plist build/component.plist \
		--install-location "/Library/Input Methods" --scripts pkg \
		--identifier com.oljk.inputmethod.KazType --version $(VERSION) $(PKG)
	@echo "Built $(PKG)"

clean:
	rm -rf build .build
