import 'package:flutter/widgets.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/core/managers/window_manager_provider.dart';
import 'package:maccy/features/history/providers/history_providers.dart';
import 'package:maccy/features/history/providers/popup_state_provider.dart';
import 'package:maccy/features/history/repositories/history_repository.dart';
import 'package:maccy/features/history/ui/history_intents.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';

class NavigateDownAction extends Action<NavigateDownIntent> {
  NavigateDownAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(NavigateDownIntent intent) {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    final totalItems = history.length;
    final showFooter = ref.read(showFooterMenuProvider);
    final menuCount = showFooter ? 3 : 0;
    final maxIdx = totalItems + menuCount - 1;

    if (maxIdx < 0) return;

    ref.read(historySelectedIndexProvider.notifier).update((val) => (val + 1).clamp(0, maxIdx));
  }
}

class NavigateUpAction extends Action<NavigateUpIntent> {
  NavigateUpAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(NavigateUpIntent intent) {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    final totalItems = history.length;
    final showFooter = ref.read(showFooterMenuProvider);
    final menuCount = showFooter ? 3 : 0;
    final maxIdx = totalItems + menuCount - 1;

    if (maxIdx < 0) return;

    ref.read(historySelectedIndexProvider.notifier).update((val) => (val - 1).clamp(0, maxIdx));
  }
}

class SelectItemAction extends Action<SelectItemIntent> {
  SelectItemAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(SelectItemIntent intent) {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    final selectedIndex = ref.read(historySelectedIndexProvider);
    final totalItems = history.length;

    if (selectedIndex < totalItems) {
      ref.read(historyControllerProvider.notifier).selectItem(selectedIndex);
    } else {
      final menuIdx = selectedIndex - totalItems;
      switch (menuIdx) {
        case 0:
          ref.read(historyControllerProvider.notifier).clearHistory();
        case 1:
          ref.read(appWindowManagerProvider.notifier).showSettings();
        case 2:
          ref.read(historyControllerProvider.notifier).quitApp();
      }
    }
  }
}

class CloseWindowAction extends Action<CloseWindowIntent> {
  CloseWindowAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(CloseWindowIntent intent) {
    ref.read(appWindowManagerProvider.notifier).hideHistory();
    ref.read(popupStateManagerProvider.notifier).reset();
  }
}

class TogglePinAction extends Action<TogglePinIntent> {
  TogglePinAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(TogglePinIntent intent) {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    final selectedIndex = ref.read(historySelectedIndexProvider);
    if (selectedIndex < history.length) {
      ref.read(historyControllerProvider.notifier).togglePin(selectedIndex);
    }
  }
}

class OpenSettingsAction extends Action<OpenSettingsIntent> {
  OpenSettingsAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(OpenSettingsIntent intent) {
    ref.read(appWindowManagerProvider.notifier).showSettings();
  }
}

class QuitAppAction extends Action<QuitAppIntent> {
  QuitAppAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(QuitAppIntent intent) {
    ref.read(historyControllerProvider.notifier).quitApp();
  }
}

class QuickSelectAction extends Action<QuickSelectIntent> {
  QuickSelectAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(QuickSelectIntent intent) {
    final history = ref.read(filteredHistoryProvider).value ?? [];
    if (intent.index < history.length) {
      ref.read(historyControllerProvider.notifier).selectItem(intent.index);
    }
  }
}

class QuickPinSelectAction extends Action<QuickPinSelectIntent> {
  QuickPinSelectAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(QuickPinSelectIntent intent) {
    ref.read(historyRepositoryProvider).getItemByPin(intent.key).then((item) {
      if (item != null) {
        ref.read(historyControllerProvider.notifier).paste(item.id);
      }
    });
  }
}
