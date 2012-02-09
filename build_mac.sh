#!/bin/sh

xcodebuild -IDEBuildOperationMaxNumberOfConcurrentCompileTasks=4 -project SampleD.xcodeproj clean
xcodebuild -IDEBuildOperationMaxNumberOfConcurrentCompileTasks=4 -project SampleD.xcodeproj -configuration Release
xcodebuild -IDEBuildOperationMaxNumberOfConcurrentCompileTasks=4 -project SampleD.xcodeproj -configuration Debug
