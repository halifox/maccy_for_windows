import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../providers/settings_provider.dart';

class SettingsPage extends HookConsumerWidget {
  const SettingsPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final settingsAsync = ref.watch(settingsProvider);

    return Scaffold(
      appBar: AppBar(
        title: const Text('Settings', style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold)),
        centerTitle: true,
        backgroundColor: Colors.transparent,
        elevation: 0,
      ),
      body: settingsAsync.when(
        data: (settings) => ListView(
          padding: const EdgeInsets.all(16),
          children: [
            _buildSection(
              context,
              'General',
              [
                ListTile(
                  title: const Text('Launch at startup'),
                  trailing: Switch(
                    value: settings.launchAtStartup,
                    onChanged: (val) => ref.read(settingsProvider.notifier).toggleLaunchAtStartup(val),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            _buildSection(
              context,
              'History',
              [
                ListTile(
                  title: const Text('History limit'),
                  subtitle: Text('${settings.historyLimit} items'),
                  trailing: SizedBox(
                    width: 100,
                    child: Slider(
                      value: settings.historyLimit.toDouble(),
                      min: 50,
                      max: 1000,
                      divisions: 19,
                      onChanged: (val) => ref.read(settingsProvider.notifier).updateHistoryLimit(val.toInt()),
                    ),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 24),
            Center(
              child: Text(
                'HaliClip v1.0.0',
                style: Theme.of(context).textTheme.labelSmall?.copyWith(color: Colors.grey),
              ),
            ),
          ],
        ),
        loading: () => const Center(child: CircularProgressIndicator()),
        error: (err, stack) => Center(child: Text('Error: $err')),
      ),
    );
  }

  Widget _buildSection(BuildContext context, String title, List<Widget> children) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
          child: Text(
            title.toUpperCase(),
            style: Theme.of(context).textTheme.labelMedium?.copyWith(
                  fontWeight: FontWeight.bold,
                  color: Theme.of(context).colorScheme.primary,
                ),
          ),
        ),
        Card(
          elevation: 0,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(12),
            side: BorderSide(color: Theme.of(context).colorScheme.outlineVariant, width: 0.5),
          ),
          child: Column(children: children),
        ),
      ],
    );
  }
}