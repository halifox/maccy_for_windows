import 'package:flutter/services.dart';
import 'package:flutter/widgets.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:maccy/features/history/providers/history_providers.dart';
import 'package:maccy/features/history/providers/popup_state_provider.dart';

class ModifierKeyHandler extends StatefulWidget {
  const ModifierKeyHandler({required this.child, super.key});

  final Widget child;

  @override
  State<ModifierKeyHandler> createState() => _ModifierKeyHandlerState();
}

class _ModifierKeyHandlerState extends State<ModifierKeyHandler> {
  @override
  Widget build(BuildContext context) {
    return Consumer(
      builder: (context, ref, child) {
        return KeyboardListener(
          focusNode: FocusNode(),
          onKeyEvent: (event) {
            if (event is KeyUpEvent) {
              final modifierKeys = ref.read(modifierKeysStateProvider);
              final popupState = ref.read(popupStateManagerProvider.notifier);

              if (popupState.handleFlagsChanged(modifierKeys.isEmpty)) {
                final selectedIndex = ref.read(historySelectedIndexProvider);
                ref.read(historyControllerProvider.notifier).selectItem(selectedIndex);
              }
            }

            if (event is! KeyDownEvent) return;

            final isAltPressed = HardwareKeyboard.instance.isAltPressed;
            if (!isAltPressed) return;

            final logicalKey = event.logicalKey;
            final char = logicalKey.keyLabel.toLowerCase();

            if (char.length == 1 && char.codeUnitAt(0) >= 97 && char.codeUnitAt(0) <= 122) {
              final history = ref.read(filteredHistoryProvider).value ?? [];
              final pinnedItem = history.firstWhere(
                (item) => item.pin == char,
                orElse: () => history.first,
              );
              if (pinnedItem.pin == char) {
                final index = history.indexOf(pinnedItem);
                if (index >= 0) {
                  ref.read(historyControllerProvider.notifier).selectItem(index);
                }
              }
            }
          },
          child: child!,
        );
      },
      child: widget.child,
    );
  }
}
