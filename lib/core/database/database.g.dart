// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'database.dart';

// ignore_for_file: type=lint
class $ClipboardEntriesTable extends ClipboardEntries
    with TableInfo<$ClipboardEntriesTable, ClipboardEntry> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $ClipboardEntriesTable(this.attachedDatabase, [this._alias]);
  static const VerificationMeta _idMeta = const VerificationMeta('id');
  @override
  late final GeneratedColumn<int> id = GeneratedColumn<int>(
    'id',
    aliasedName,
    false,
    hasAutoIncrement: true,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'PRIMARY KEY AUTOINCREMENT',
    ),
  );
  static const VerificationMeta _contentMeta = const VerificationMeta(
    'content',
  );
  @override
  late final GeneratedColumn<String> content = GeneratedColumn<String>(
    'content',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
    defaultConstraints: GeneratedColumn.constraintIsAlways('UNIQUE'),
  );
  static const VerificationMeta _typeMeta = const VerificationMeta('type');
  @override
  late final GeneratedColumn<String> type = GeneratedColumn<String>(
    'type',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('text'),
  );
  static const VerificationMeta _createdAtMeta = const VerificationMeta(
    'createdAt',
  );
  @override
  late final GeneratedColumn<DateTime> createdAt = GeneratedColumn<DateTime>(
    'created_at',
    aliasedName,
    false,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: false,
    defaultValue: currentDateAndTime,
  );
  static const VerificationMeta _isPinnedMeta = const VerificationMeta(
    'isPinned',
  );
  @override
  late final GeneratedColumn<bool> isPinned = GeneratedColumn<bool>(
    'is_pinned',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("is_pinned" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _pinOrderMeta = const VerificationMeta(
    'pinOrder',
  );
  @override
  late final GeneratedColumn<int> pinOrder = GeneratedColumn<int>(
    'pin_order',
    aliasedName,
    true,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
  );
  @override
  List<GeneratedColumn> get $columns => [
    id,
    content,
    type,
    createdAt,
    isPinned,
    pinOrder,
  ];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'clipboard_entries';
  @override
  VerificationContext validateIntegrity(
    Insertable<ClipboardEntry> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('content')) {
      context.handle(
        _contentMeta,
        content.isAcceptableOrUnknown(data['content']!, _contentMeta),
      );
    } else if (isInserting) {
      context.missing(_contentMeta);
    }
    if (data.containsKey('type')) {
      context.handle(
        _typeMeta,
        type.isAcceptableOrUnknown(data['type']!, _typeMeta),
      );
    }
    if (data.containsKey('created_at')) {
      context.handle(
        _createdAtMeta,
        createdAt.isAcceptableOrUnknown(data['created_at']!, _createdAtMeta),
      );
    }
    if (data.containsKey('is_pinned')) {
      context.handle(
        _isPinnedMeta,
        isPinned.isAcceptableOrUnknown(data['is_pinned']!, _isPinnedMeta),
      );
    }
    if (data.containsKey('pin_order')) {
      context.handle(
        _pinOrderMeta,
        pinOrder.isAcceptableOrUnknown(data['pin_order']!, _pinOrderMeta),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  ClipboardEntry map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return ClipboardEntry(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      content: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}content'],
      )!,
      type: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}type'],
      )!,
      createdAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}created_at'],
      )!,
      isPinned: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}is_pinned'],
      )!,
      pinOrder: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}pin_order'],
      ),
    );
  }

  @override
  $ClipboardEntriesTable createAlias(String alias) {
    return $ClipboardEntriesTable(attachedDatabase, alias);
  }
}

class ClipboardEntry extends DataClass implements Insertable<ClipboardEntry> {
  final int id;
  final String content;
  final String type;
  final DateTime createdAt;
  final bool isPinned;
  final int? pinOrder;
  const ClipboardEntry({
    required this.id,
    required this.content,
    required this.type,
    required this.createdAt,
    required this.isPinned,
    this.pinOrder,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['content'] = Variable<String>(content);
    map['type'] = Variable<String>(type);
    map['created_at'] = Variable<DateTime>(createdAt);
    map['is_pinned'] = Variable<bool>(isPinned);
    if (!nullToAbsent || pinOrder != null) {
      map['pin_order'] = Variable<int>(pinOrder);
    }
    return map;
  }

  ClipboardEntriesCompanion toCompanion(bool nullToAbsent) {
    return ClipboardEntriesCompanion(
      id: Value(id),
      content: Value(content),
      type: Value(type),
      createdAt: Value(createdAt),
      isPinned: Value(isPinned),
      pinOrder: pinOrder == null && nullToAbsent
          ? const Value.absent()
          : Value(pinOrder),
    );
  }

  factory ClipboardEntry.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return ClipboardEntry(
      id: serializer.fromJson<int>(json['id']),
      content: serializer.fromJson<String>(json['content']),
      type: serializer.fromJson<String>(json['type']),
      createdAt: serializer.fromJson<DateTime>(json['createdAt']),
      isPinned: serializer.fromJson<bool>(json['isPinned']),
      pinOrder: serializer.fromJson<int?>(json['pinOrder']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'content': serializer.toJson<String>(content),
      'type': serializer.toJson<String>(type),
      'createdAt': serializer.toJson<DateTime>(createdAt),
      'isPinned': serializer.toJson<bool>(isPinned),
      'pinOrder': serializer.toJson<int?>(pinOrder),
    };
  }

  ClipboardEntry copyWith({
    int? id,
    String? content,
    String? type,
    DateTime? createdAt,
    bool? isPinned,
    Value<int?> pinOrder = const Value.absent(),
  }) => ClipboardEntry(
    id: id ?? this.id,
    content: content ?? this.content,
    type: type ?? this.type,
    createdAt: createdAt ?? this.createdAt,
    isPinned: isPinned ?? this.isPinned,
    pinOrder: pinOrder.present ? pinOrder.value : this.pinOrder,
  );
  ClipboardEntry copyWithCompanion(ClipboardEntriesCompanion data) {
    return ClipboardEntry(
      id: data.id.present ? data.id.value : this.id,
      content: data.content.present ? data.content.value : this.content,
      type: data.type.present ? data.type.value : this.type,
      createdAt: data.createdAt.present ? data.createdAt.value : this.createdAt,
      isPinned: data.isPinned.present ? data.isPinned.value : this.isPinned,
      pinOrder: data.pinOrder.present ? data.pinOrder.value : this.pinOrder,
    );
  }

  @override
  String toString() {
    return (StringBuffer('ClipboardEntry(')
          ..write('id: $id, ')
          ..write('content: $content, ')
          ..write('type: $type, ')
          ..write('createdAt: $createdAt, ')
          ..write('isPinned: $isPinned, ')
          ..write('pinOrder: $pinOrder')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode =>
      Object.hash(id, content, type, createdAt, isPinned, pinOrder);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is ClipboardEntry &&
          other.id == this.id &&
          other.content == this.content &&
          other.type == this.type &&
          other.createdAt == this.createdAt &&
          other.isPinned == this.isPinned &&
          other.pinOrder == this.pinOrder);
}

class ClipboardEntriesCompanion extends UpdateCompanion<ClipboardEntry> {
  final Value<int> id;
  final Value<String> content;
  final Value<String> type;
  final Value<DateTime> createdAt;
  final Value<bool> isPinned;
  final Value<int?> pinOrder;
  const ClipboardEntriesCompanion({
    this.id = const Value.absent(),
    this.content = const Value.absent(),
    this.type = const Value.absent(),
    this.createdAt = const Value.absent(),
    this.isPinned = const Value.absent(),
    this.pinOrder = const Value.absent(),
  });
  ClipboardEntriesCompanion.insert({
    this.id = const Value.absent(),
    required String content,
    this.type = const Value.absent(),
    this.createdAt = const Value.absent(),
    this.isPinned = const Value.absent(),
    this.pinOrder = const Value.absent(),
  }) : content = Value(content);
  static Insertable<ClipboardEntry> custom({
    Expression<int>? id,
    Expression<String>? content,
    Expression<String>? type,
    Expression<DateTime>? createdAt,
    Expression<bool>? isPinned,
    Expression<int>? pinOrder,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (content != null) 'content': content,
      if (type != null) 'type': type,
      if (createdAt != null) 'created_at': createdAt,
      if (isPinned != null) 'is_pinned': isPinned,
      if (pinOrder != null) 'pin_order': pinOrder,
    });
  }

  ClipboardEntriesCompanion copyWith({
    Value<int>? id,
    Value<String>? content,
    Value<String>? type,
    Value<DateTime>? createdAt,
    Value<bool>? isPinned,
    Value<int?>? pinOrder,
  }) {
    return ClipboardEntriesCompanion(
      id: id ?? this.id,
      content: content ?? this.content,
      type: type ?? this.type,
      createdAt: createdAt ?? this.createdAt,
      isPinned: isPinned ?? this.isPinned,
      pinOrder: pinOrder ?? this.pinOrder,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (content.present) {
      map['content'] = Variable<String>(content.value);
    }
    if (type.present) {
      map['type'] = Variable<String>(type.value);
    }
    if (createdAt.present) {
      map['created_at'] = Variable<DateTime>(createdAt.value);
    }
    if (isPinned.present) {
      map['is_pinned'] = Variable<bool>(isPinned.value);
    }
    if (pinOrder.present) {
      map['pin_order'] = Variable<int>(pinOrder.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('ClipboardEntriesCompanion(')
          ..write('id: $id, ')
          ..write('content: $content, ')
          ..write('type: $type, ')
          ..write('createdAt: $createdAt, ')
          ..write('isPinned: $isPinned, ')
          ..write('pinOrder: $pinOrder')
          ..write(')'))
        .toString();
  }
}

abstract class _$AppDatabase extends GeneratedDatabase {
  _$AppDatabase(QueryExecutor e) : super(e);
  $AppDatabaseManager get managers => $AppDatabaseManager(this);
  late final $ClipboardEntriesTable clipboardEntries = $ClipboardEntriesTable(
    this,
  );
  @override
  Iterable<TableInfo<Table, Object?>> get allTables =>
      allSchemaEntities.whereType<TableInfo<Table, Object?>>();
  @override
  List<DatabaseSchemaEntity> get allSchemaEntities => [clipboardEntries];
}

typedef $$ClipboardEntriesTableCreateCompanionBuilder =
    ClipboardEntriesCompanion Function({
      Value<int> id,
      required String content,
      Value<String> type,
      Value<DateTime> createdAt,
      Value<bool> isPinned,
      Value<int?> pinOrder,
    });
typedef $$ClipboardEntriesTableUpdateCompanionBuilder =
    ClipboardEntriesCompanion Function({
      Value<int> id,
      Value<String> content,
      Value<String> type,
      Value<DateTime> createdAt,
      Value<bool> isPinned,
      Value<int?> pinOrder,
    });

class $$ClipboardEntriesTableFilterComposer
    extends Composer<_$AppDatabase, $ClipboardEntriesTable> {
  $$ClipboardEntriesTableFilterComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  ColumnFilters<int> get id => $composableBuilder(
    column: $table.id,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get content => $composableBuilder(
    column: $table.content,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get type => $composableBuilder(
    column: $table.type,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get createdAt => $composableBuilder(
    column: $table.createdAt,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get isPinned => $composableBuilder(
    column: $table.isPinned,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get pinOrder => $composableBuilder(
    column: $table.pinOrder,
    builder: (column) => ColumnFilters(column),
  );
}

class $$ClipboardEntriesTableOrderingComposer
    extends Composer<_$AppDatabase, $ClipboardEntriesTable> {
  $$ClipboardEntriesTableOrderingComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  ColumnOrderings<int> get id => $composableBuilder(
    column: $table.id,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get content => $composableBuilder(
    column: $table.content,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get type => $composableBuilder(
    column: $table.type,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get createdAt => $composableBuilder(
    column: $table.createdAt,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get isPinned => $composableBuilder(
    column: $table.isPinned,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get pinOrder => $composableBuilder(
    column: $table.pinOrder,
    builder: (column) => ColumnOrderings(column),
  );
}

class $$ClipboardEntriesTableAnnotationComposer
    extends Composer<_$AppDatabase, $ClipboardEntriesTable> {
  $$ClipboardEntriesTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  GeneratedColumn<int> get id =>
      $composableBuilder(column: $table.id, builder: (column) => column);

  GeneratedColumn<String> get content =>
      $composableBuilder(column: $table.content, builder: (column) => column);

  GeneratedColumn<String> get type =>
      $composableBuilder(column: $table.type, builder: (column) => column);

  GeneratedColumn<DateTime> get createdAt =>
      $composableBuilder(column: $table.createdAt, builder: (column) => column);

  GeneratedColumn<bool> get isPinned =>
      $composableBuilder(column: $table.isPinned, builder: (column) => column);

  GeneratedColumn<int> get pinOrder =>
      $composableBuilder(column: $table.pinOrder, builder: (column) => column);
}

class $$ClipboardEntriesTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $ClipboardEntriesTable,
          ClipboardEntry,
          $$ClipboardEntriesTableFilterComposer,
          $$ClipboardEntriesTableOrderingComposer,
          $$ClipboardEntriesTableAnnotationComposer,
          $$ClipboardEntriesTableCreateCompanionBuilder,
          $$ClipboardEntriesTableUpdateCompanionBuilder,
          (
            ClipboardEntry,
            BaseReferences<
              _$AppDatabase,
              $ClipboardEntriesTable,
              ClipboardEntry
            >,
          ),
          ClipboardEntry,
          PrefetchHooks Function()
        > {
  $$ClipboardEntriesTableTableManager(
    _$AppDatabase db,
    $ClipboardEntriesTable table,
  ) : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$ClipboardEntriesTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$ClipboardEntriesTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$ClipboardEntriesTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<String> content = const Value.absent(),
                Value<String> type = const Value.absent(),
                Value<DateTime> createdAt = const Value.absent(),
                Value<bool> isPinned = const Value.absent(),
                Value<int?> pinOrder = const Value.absent(),
              }) => ClipboardEntriesCompanion(
                id: id,
                content: content,
                type: type,
                createdAt: createdAt,
                isPinned: isPinned,
                pinOrder: pinOrder,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required String content,
                Value<String> type = const Value.absent(),
                Value<DateTime> createdAt = const Value.absent(),
                Value<bool> isPinned = const Value.absent(),
                Value<int?> pinOrder = const Value.absent(),
              }) => ClipboardEntriesCompanion.insert(
                id: id,
                content: content,
                type: type,
                createdAt: createdAt,
                isPinned: isPinned,
                pinOrder: pinOrder,
              ),
          withReferenceMapper: (p0) => p0
              .map((e) => (e.readTable(table), BaseReferences(db, table, e)))
              .toList(),
          prefetchHooksCallback: null,
        ),
      );
}

typedef $$ClipboardEntriesTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $ClipboardEntriesTable,
      ClipboardEntry,
      $$ClipboardEntriesTableFilterComposer,
      $$ClipboardEntriesTableOrderingComposer,
      $$ClipboardEntriesTableAnnotationComposer,
      $$ClipboardEntriesTableCreateCompanionBuilder,
      $$ClipboardEntriesTableUpdateCompanionBuilder,
      (
        ClipboardEntry,
        BaseReferences<_$AppDatabase, $ClipboardEntriesTable, ClipboardEntry>,
      ),
      ClipboardEntry,
      PrefetchHooks Function()
    >;

class $AppDatabaseManager {
  final _$AppDatabase _db;
  $AppDatabaseManager(this._db);
  $$ClipboardEntriesTableTableManager get clipboardEntries =>
      $$ClipboardEntriesTableTableManager(_db, _db.clipboardEntries);
}
