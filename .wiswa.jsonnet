{
  uses_user_defaults: true,
  project_type: 'c',
  project_name: 'libtether',
  version: '0.0.0',
  security_policy_supported_versions: { '0.0.x': ':white_check_mark:' },
  description: 'Tools and a C library to attach and detach macOS disk images, replicating hdiutil via the private DiskImages framework and DiskArbitration.',
  keywords: ['dmg', 'disk image', 'diskarbitration', 'diskimages', 'hdiutil', 'macos'],
  want_main: false,
  clang_format_args: 'include/*.h src/*.c tools/*.c',
  vscode+: {
    c_cpp+: {
      configurations: [
        {
          cStandard: 'gnu17',
          compilerPath: '/usr/bin/clang',
          cppStandard: 'gnu++17',
          includePath: [
            '${workspaceFolder}/include/**',
            '${workspaceFolder}/src/**',
            '${workspaceFolder}/tools/**',
          ],
          name: 'Mac',
        },
      ],
    },
  },
}
