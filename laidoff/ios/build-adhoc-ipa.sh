#!/bin/bash
#security unlock-keychain -p ${!KEYCHAIN_PASSWORD}
xcodebuild -scheme laidoff clean archive -archivePath build/laidoff
xcodebuild -exportArchive -exportOptionsPlist exportoptions.plist -archivePath "build/laidoff.xcarchive" -exportPath "build"
