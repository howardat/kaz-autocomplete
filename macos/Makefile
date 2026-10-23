# Build and install the KazType input method. Needs only Command Line Tools.
#   make install   build, sign, copy to ~/Library/Input Methods, register
#   make test      unit tests for KazCore

APP      := build/KazType.app
DEST     := $(HOME)/Library/Input Methods
BIN      := $(shell swift build -c release --show-bin-path)/KazType

.PHONY: all app test install uninstall clean

all: app

app:
	swift build -c release --product KazType
	@test -f Resources/vocab.json || (echo "Missing model. Run: uv run python -m ml.export" && exit 1)
	rm -rf $(APP)
	mkdir -p $(APP)/Contents/MacOS $(APP)/Contents/Resources
	cp $(BIN) $(APP)/Contents/MacOS/KazType
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

clean:
	rm -rf build .build
