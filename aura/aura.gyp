# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'aura',
      'type': '<(component)',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../../skia/skia.gyp:skia',
        '../gfx/compositor/compositor.gyp:compositor',
        '../ui.gyp:gfx_resources',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
      ],
      'defines': [
        'AURA_IMPLEMENTATION',
      ],
      'sources': [
        'aura_switches.cc',
        'aura_switches.h',
        'client/activation_client.cc',
        'client/activation_client.h',
        'client/activation_delegate.cc',
        'client/activation_delegate.h',
        'client/aura_constants.cc',
        'client/aura_constants.h',
        'client/dispatcher_client.cc',
        'client/dispatcher_client.h',
        'client/drag_drop_client.cc',
        'client/drag_drop_client.h',
        'client/drag_drop_delegate.cc',
        'client/drag_drop_delegate.h',
        'client/event_client.cc',
        'client/event_client.h',
        'client/stacking_client.cc',
        'client/stacking_client.h',
        'client/tooltip_client.cc',
        'client/tooltip_client.h',
        'client/visibility_client.cc',
        'client/visibility_client.h',
        'client/window_move_client.cc',
        'client/window_move_client.h',
        'client/window_types.h',
        'cursor.h',
        'dispatcher_linux.cc',
        'dispatcher_linux.h',
        'dispatcher_win.cc',
        'env.cc',
        'env.h',
        'env_observer.h',
        'event.cc',
        'event.h',
        'event_filter.h',
        'event_mac.mm',
        'event_mac.h',
        'focus_manager.h',
        'gestures/gesture_configuration.cc',
        'gestures/gesture_configuration.h',
        'gestures/gesture_recognizer.h',
        'gestures/gesture_recognizer_aura.cc',
        'gestures/gesture_recognizer_aura.h',
        'gestures/gesture_point.cc',
        'gestures/gesture_point.h',
        'gestures/velocity_calculator.cc',
        'gestures/velocity_calculator.h',
        'gestures/gesture_sequence.cc',
        'gestures/gesture_sequence.h',
        'layout_manager.cc',
        'layout_manager.h',
        'monitor.cc',
        'monitor.h',
        'monitor_change_observer_x11.cc',
        'monitor_change_observer_x11.h',
        'monitor_manager.cc',
        'monitor_manager.h',
        'single_monitor_manager.h',
        'single_monitor_manager.cc',
        'root_window_host.h',
        'root_window_host_linux.cc',
        'root_window_host_linux.h',
        'root_window_host_mac.h',
        'root_window_host_mac.mm',
        'root_window_host_win.cc',
        'root_window_host_win.h',
        'root_window_mac.h',
        'root_window_mac.mm',
        'root_window_view_mac.h',
        'root_window_view_mac.mm',
        'root_window.cc',
        'root_window.h',
        'ui_controls_win.cc',
        'ui_controls_x11.cc',
        'window.cc',
        'window.h',
        'window_delegate.h',
        'window_observer.h',
      ],
      'conditions': [
        ['OS=="mac"', {
          'sources/': [
            ['exclude', 'client/dispatcher_client.cc'],
            ['exclude', 'client/dispatcher_client.h'],
          ],
        }],
        ['OS=="linux"', {
          'link_settings': {
            'libraries': [
              '-lXfixes',
              '-lXrandr',
            ],
          },
        }],
      ],
    },
    {
      'target_name': 'test_support_aura',
      'type': 'static_library',
      'dependencies': [
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../ui.gyp:ui',
        'aura',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'test/aura_test_base.cc',
        'test/aura_test_base.h',
        'test/aura_test_helper.cc',
        'test/aura_test_helper.h',
        'test/event_generator.cc',
        'test/event_generator.h',
        'test/test_activation_client.cc',
        'test/test_activation_client.h',
        'test/test_event_filter.cc',
        'test/test_event_filter.h',
        'test/test_screen.cc',
        'test/test_screen.h',
        'test/test_stacking_client.cc',
        'test/test_stacking_client.h',
        'test/test_windows.cc',
        'test/test_windows.h',
        'test/test_window_delegate.cc',
        'test/test_window_delegate.h',
      ],
    },
    {
      'target_name': 'aura_demo',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:base',
        '../../base/base.gyp:base_i18n',
        '../../skia/skia.gyp:skia',
        '../../third_party/icu/icu.gyp:icui18n',
        '../../third_party/icu/icu.gyp:icuuc',
        '../gfx/compositor/compositor.gyp:compositor',
        '../gfx/compositor/compositor.gyp:compositor_test_support',
        '../ui.gyp:gfx_resources',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        'aura',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'demo/demo_main.cc',
        '<(SHARED_INTERMEDIATE_DIR)/ui/gfx/gfx_resources.rc',
        '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources/ui_resources.rc',
      ],
    },
    {
      'target_name': 'aura_unittests',
      'type': 'executable',
      'dependencies': [
        '../../base/base.gyp:test_support_base',
        '../../chrome/chrome_resources.gyp:packed_resources',
        '../../skia/skia.gyp:skia',
        '../../testing/gtest.gyp:gtest',
        '../gfx/compositor/compositor.gyp:compositor_test_support',
        '../gfx/compositor/compositor.gyp:compositor',
        '../gfx/gl/gl.gyp:gl',
        '../ui.gyp:gfx_resources',
        '../ui.gyp:ui',
        '../ui.gyp:ui_resources',
        'test_support_aura',
        'aura',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'gestures/gesture_recognizer_unittest.cc',
        'gestures/velocity_calculator_unittest.cc',
        'test/run_all_unittests.cc',
        'test/test_suite.cc',
        'test/test_suite.h',
        'root_window_unittest.cc',
        'event_filter_unittest.cc',
        'event_unittest.cc',
        'window_unittest.cc',
        '<(SHARED_INTERMEDIATE_DIR)/ui/gfx/gfx_resources.rc',
        '<(SHARED_INTERMEDIATE_DIR)/ui/ui_resources/ui_resources.rc',
      ],
      'conditions': [
        # osmesa GL implementation is used on linux.
        ['OS=="linux"', {
          'dependencies': [
            '<(DEPTH)/third_party/mesa/mesa.gyp:osmesa',
          ],
        }],
      ],
    },
  ],
}
