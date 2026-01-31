import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import '../../../core/database/database_provider.dart';
import '../../../core/managers/window_manager_provider.dart';
import 'package:flutter/services.dart';
import 'package:drift/drift.dart' show OrderingTerm;

class HistoryPage extends HookConsumerWidget {
  const HistoryPage({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final db = ref.watch(appDatabaseProvider);
    final historyStream = useMemoized(() => (db.select(db.clipboardEntries)
      ..orderBy([(t) => OrderingTerm.desc(t.createdAt)])
      ..limit(50))
      .watch(), [db]);
    final history = useStream(historyStream);

    return Scaffold(
      backgroundColor: Colors.transparent,
      body: Container(
        decoration: BoxDecoration(
          color: Theme.of(context).colorScheme.surface.withOpacity(0.9),
          borderRadius: BorderRadius.circular(12),
          border: Border.all(
            color: Theme.of(context).colorScheme.outlineVariant,
            width: 0.5,
          ),
        ),
        child: Column(
          children: [
            Padding(
              padding: const EdgeInsets.all(8.0),
              child: TextField(
                decoration: InputDecoration(
                  hintText: 'Search...',
                  prefixIcon: const Icon(Icons.search, size: 18),
                  isDense: true,
                  contentPadding: const EdgeInsets.symmetric(vertical: 8),
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(8),
                  ),
                ),
                autofocus: true,
              ),
            ),
            Expanded(
              child: ListView.builder(
                itemCount: history.data?.length ?? 0,
                itemBuilder: (context, index) {
                  final item = history.data![index];
                  return ListTile(
                    dense: true,
                    title: Text(
                      item.content,
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                    ),
                    onTap: () async {
                      await Clipboard.setData(ClipboardData(text: item.content));
                      ref.read(appWindowManagerProvider.notifier).hide();
                    },
                  );
                },
              ),
            ),
          ],
        ),
      ),
    );
  }
}
