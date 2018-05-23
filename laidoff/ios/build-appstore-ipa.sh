#!/bin/bash
#security unlock-keychain -p ${!KEYCHAIN_PASSWORD}
xcodebuild -scheme laidoff clean archive -archivePath build-appstore/laidoff
xcodebuild -exportArchive -exportOptionsPlist exportoptions-appstore.plist -archivePath "build-appstore/laidoff.xcarchive" -exportPath "build-appstore"
