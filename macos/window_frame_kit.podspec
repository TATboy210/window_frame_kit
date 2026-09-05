#
# To learn more about a Podspec see http://guides.cocoapods.org/syntax/podspec.html.
# Run `pod lib lint window_frame_kit.podspec` to validate before publishing.
#
Pod::Spec.new do |s|
  s.name             = 'window_frame_kit'
  s.version          = '0.0.1'
  # 占位符清理（研究 Pitfall 7）：summary/description 与 pubspec description 同义
  s.summary          = 'Frame takeover and complete window management for Flutter desktop apps.'
  s.description      = <<-DESC
Frame takeover and complete window management events for Flutter desktop apps (Windows, Linux, macOS): frameless windows, four-edge native resizing, and full window-manager parity APIs.
                       DESC
  s.homepage         = 'https://github.com/TATboy210/window_frame_kit'
  s.license          = { :file => '../LICENSE' }
  s.author           = { 'TATboy210' => '354900184@qq.com' }

  s.source           = { :path => '.' }
  s.source_files = 'window_frame_kit/Sources/window_frame_kit/**/*'

  # If your plugin requires a privacy manifest, for example if it collects user
  # data, update the PrivacyInfo.xcprivacy file to describe your plugin's
  # privacy impact, and then uncomment this line. For more information,
  # see https://developer.apple.com/documentation/bundleresources/privacy_manifest_files
  # s.resource_bundles = {'window_frame_kit_privacy' => ['window_frame_kit/Sources/window_frame_kit/PrivacyInfo.xcprivacy']}

  s.dependency 'FlutterMacOS'

  # 与现有 SwiftPM/Flutter 模板一致，不能宣称未验证的旧系统支持。
  s.platform = :osx, '12.0'
  s.pod_target_xcconfig = { 'DEFINES_MODULE' => 'YES' }
  s.swift_version = '5.0'
end
