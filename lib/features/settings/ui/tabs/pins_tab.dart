import 'package:clipboard/features/settings/providers/pins_provider.dart';
import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../widgets/macos_settings_widgets.dart';

class PinsTab extends ConsumerWidget {
  const PinsTab({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final pinsAsync = ref.watch(pinsProvider);
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return Column(
      children: [
        MacosSettingsGroup(
          title: 'Fixed Snippets',
          children: [
            pinsAsync.when(
              data: (pins) => pins.isEmpty
                  ? const _EmptyPins()
                  : ListView.builder(
                      shrinkWrap: true,
                      physics: const NeverScrollableScrollPhysics(),
                      itemCount: pins.length,
                      itemBuilder: (context, index) {
                        final pin = pins[index];
                        return MacosSettingsTile(
                          label: pin.title,
                          subtitle: pin.content,
                          icon: CupertinoIcons.pin_fill,
                          iconColor: CupertinoColors.systemBlue,
                          trailing: CupertinoButton(
                            padding: EdgeInsets.zero,
                            child: const Icon(CupertinoIcons.minus_circle, color: CupertinoColors.systemRed, size: 20),
                            onPressed: () => ref.read(pinsProvider.notifier).deletePin(pin.id),
                          ),
                        );
                      },
                    ),
              loading: () => const Center(child: CupertinoActivityIndicator()),
              error: (err, _) => Center(child: Text('Error: $err')),
            ),
          ],
        ),
        const SizedBox(height: 8),
        _AddPinSection(),
      ],
    );
  }
}

class _EmptyPins extends StatelessWidget {
  const _EmptyPins();
  @override
  Widget build(BuildContext context) {
    return const Padding(
      padding: EdgeInsets.all(24.0),
      child: Center(
        child: Text('No pins created yet.\nAdd frequently used text below.', 
          textAlign: TextAlign.center,
          style: TextStyle(fontSize: 12, color: CupertinoColors.systemGrey)),
      ),
    );
  }
}

class _AddPinSection extends HookConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final titleController = useTextEditingController();
    final contentController = useTextEditingController();
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return MacosSettingsGroup(
      title: 'Create New Pin',
      children: [
        Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            children: [
              CupertinoTextField(
                controller: titleController,
                placeholder: 'Title (e.g. My Email)',
                decoration: BoxDecoration(color: isDark ? Colors.white10 : Colors.black.withOpacity(0.05), borderRadius: BorderRadius.circular(6)),
                style: TextStyle(fontSize: 13, color: isDark ? Colors.white : Colors.black),
              ),
              const SizedBox(height: 10),
              CupertinoTextField(
                controller: contentController,
                placeholder: 'Content',
                maxLines: 3,
                decoration: BoxDecoration(color: isDark ? Colors.white10 : Colors.black.withOpacity(0.05), borderRadius: BorderRadius.circular(6)),
                style: TextStyle(fontSize: 13, color: isDark ? Colors.white : Colors.black),
              ),
              const SizedBox(height: 12),
              CupertinoButton.filled(
                padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 8),
                minSize: 0,
                child: const Text('Add Pin', style: TextStyle(fontSize: 13, fontWeight: FontWeight.w600)),
                onPressed: () {
                  if (titleController.text.isNotEmpty && contentController.text.isNotEmpty) {
                    ref.read(pinsProvider.notifier).addPin(titleController.text, contentController.text);
                    titleController.clear();
                    contentController.clear();
                  }
                },
              ),
            ],
          ),
        ),
      ],
    );
  }
}