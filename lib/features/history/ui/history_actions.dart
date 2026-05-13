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
    ref.read(historyControllerProvider.notifier).selectNext();
  }
}

class NavigateUpAction extends Action<NavigateUpIntent> {
  NavigateUpAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(NavigateUpIntent intent) {
    ref.read(historyControllerProvider.notifier).selectPrevious();
  }
}

class SelectItemAction extends Action<SelectItemIntent> {
  SelectItemAction(this.ref);

  final WidgetRef ref;

  @override
  void invoke(SelectItemIntent intent) {
    final selectedId = ref.read(historySelectedIdProvider);
    if (selectedId != null) {
      ref.read(historyControllerProvider.notifier).selectItem(selectedId);
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
    final selectedId = ref.read(historySelectedIdProvider);
    if (selectedId != null) {
      ref.read(historyControllerProvider.notifier).togglePin(selectedId);
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
    final unpinnedItems = ref.read(unpinnedHistoryProvider).value ?? [];
    if (intent.index < unpinnedItems.length) {
      ref.read(historyControllerProvider.notifier).selectItem(unpinnedItems[intent.index].id);
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
