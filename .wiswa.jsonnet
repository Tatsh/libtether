{
  uses_user_defaults: true,
  project_type: 'c',
  project_name: 'libtether',
  version: '0.0.1',
  security_policy_supported_versions: { '0.0.x': ':white_check_mark:' },
  description: 'Programmatically attach a DMG without a hdiutil subprocess.',
  keywords: ['dmg', 'disk image', 'diskarbitration', 'diskimages', 'hdiutil', 'macos'],
  want_main: false,
  // The tests are C/cmocka, not the Python suite wiswa would scaffold; tests.yml is maintained by
  // hand in this project.
  want_tests: false,
  // No WinGet publishing; this is a macOS-only project.
  want_winget: false,
  clang_format_args: 'include/*.h src/*.c tools/*.c tests/*.c tests/*.h',
  // Keep LIBTETHER_VERSION in the public header in sync on release bumps.
  cz+: {
    commitizen+: {
      version_files+: ['include/libtether.h'],
    },
  },
  vcpkg+: {
    dependencies+: ['cmocka'],
  },
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
+ {
  want_scan_build: true,
  github+: { workflows+: {
    scan_build+: {
      configure_command: 'cmake -S . -B build-scan -DCMAKE_BUILD_TYPE=Debug -DBUILD_TOOLS=ON',
      documentation_steps: [
        {
          name: 'Install Doxygen and Graphviz',
          run: 'sudo apt-get update && sudo apt-get install --no-install-recommends -y doxygen graphviz',
        },
        {
          name: 'Configure',
          run: 'cmake -S . -B build -DBUILD_DOCS=ON',
        },
        {
          name: 'Build documentation',
          run: 'cmake --build build --target docs',
        },
      ],
      env: {},
      runs_on: 'macos-15',
      setup_steps: [],
      workflow_file: 'pages.yml',
    },
  } },
}
