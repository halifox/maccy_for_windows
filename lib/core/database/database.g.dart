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
  @override
  List<GeneratedColumn> get $columns => [
    id,
    content,
    type,
    createdAt,
    isPinned,
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
  const ClipboardEntry({
    required this.id,
    required this.content,
    required this.type,
    required this.createdAt,
    required this.isPinned,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['content'] = Variable<String>(content);
    map['type'] = Variable<String>(type);
    map['created_at'] = Variable<DateTime>(createdAt);
    map['is_pinned'] = Variable<bool>(isPinned);
    return map;
  }

  ClipboardEntriesCompanion toCompanion(bool nullToAbsent) {
    return ClipboardEntriesCompanion(
      id: Value(id),
      content: Value(content),
      type: Value(type),
      createdAt: Value(createdAt),
      isPinned: Value(isPinned),
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
    };
  }

  ClipboardEntry copyWith({
    int? id,
    String? content,
    String? type,
    DateTime? createdAt,
    bool? isPinned,
  }) => ClipboardEntry(
    id: id ?? this.id,
    content: content ?? this.content,
    type: type ?? this.type,
    createdAt: createdAt ?? this.createdAt,
    isPinned: isPinned ?? this.isPinned,
  );
  ClipboardEntry copyWithCompanion(ClipboardEntriesCompanion data) {
    return ClipboardEntry(
      id: data.id.present ? data.id.value : this.id,
      content: data.content.present ? data.content.value : this.content,
      type: data.type.present ? data.type.value : this.type,
      createdAt: data.createdAt.present ? data.createdAt.value : this.createdAt,
      isPinned: data.isPinned.present ? data.isPinned.value : this.isPinned,
    );
  }

  @override
  String toString() {
    return (StringBuffer('ClipboardEntry(')
          ..write('id: $id, ')
          ..write('content: $content, ')
          ..write('type: $type, ')
          ..write('createdAt: $createdAt, ')
          ..write('isPinned: $isPinned')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(id, content, type, createdAt, isPinned);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is ClipboardEntry &&
          other.id == this.id &&
          other.content == this.content &&
          other.type == this.type &&
          other.createdAt == this.createdAt &&
          other.isPinned == this.isPinned);
}

class ClipboardEntriesCompanion extends UpdateCompanion<ClipboardEntry> {
  final Value<int> id;
  final Value<String> content;
  final Value<String> type;
  final Value<DateTime> createdAt;
  final Value<bool> isPinned;
  const ClipboardEntriesCompanion({
    this.id = const Value.absent(),
    this.content = const Value.absent(),
    this.type = const Value.absent(),
    this.createdAt = const Value.absent(),
    this.isPinned = const Value.absent(),
  });
  ClipboardEntriesCompanion.insert({
    this.id = const Value.absent(),
    required String content,
    this.type = const Value.absent(),
    this.createdAt = const Value.absent(),
    this.isPinned = const Value.absent(),
  }) : content = Value(content);
  static Insertable<ClipboardEntry> custom({
    Expression<int>? id,
    Expression<String>? content,
    Expression<String>? type,
    Expression<DateTime>? createdAt,
    Expression<bool>? isPinned,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (content != null) 'content': content,
      if (type != null) 'type': type,
      if (createdAt != null) 'created_at': createdAt,
      if (isPinned != null) 'is_pinned': isPinned,
    });
  }

  ClipboardEntriesCompanion copyWith({
    Value<int>? id,
    Value<String>? content,
    Value<String>? type,
    Value<DateTime>? createdAt,
    Value<bool>? isPinned,
  }) {
    return ClipboardEntriesCompanion(
      id: id ?? this.id,
      content: content ?? this.content,
      type: type ?? this.type,
      createdAt: createdAt ?? this.createdAt,
      isPinned: isPinned ?? this.isPinned,
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
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('ClipboardEntriesCompanion(')
          ..write('id: $id, ')
          ..write('content: $content, ')
          ..write('type: $type, ')
          ..write('createdAt: $createdAt, ')
          ..write('isPinned: $isPinned')
          ..write(')'))
        .toString();
  }
}

class $AppSettingsTable extends AppSettings
    with TableInfo<$AppSettingsTable, AppSetting> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $AppSettingsTable(this.attachedDatabase, [this._alias]);
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
  static const VerificationMeta _historyLimitMeta = const VerificationMeta(
    'historyLimit',
  );
  @override
  late final GeneratedColumn<int> historyLimit = GeneratedColumn<int>(
    'history_limit',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(200),
  );
  static const VerificationMeta _launchAtStartupMeta = const VerificationMeta(
    'launchAtStartup',
  );
  @override
  late final GeneratedColumn<bool> launchAtStartup = GeneratedColumn<bool>(
    'launch_at_startup',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("launch_at_startup" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _hotkeyJsonMeta = const VerificationMeta(
    'hotkeyJson',
  );
  @override
  late final GeneratedColumn<String> hotkeyJson = GeneratedColumn<String>(
    'hotkey_json',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  @override
  List<GeneratedColumn> get $columns => [
    id,
    historyLimit,
    launchAtStartup,
    hotkeyJson,
  ];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'app_settings';
  @override
  VerificationContext validateIntegrity(
    Insertable<AppSetting> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('history_limit')) {
      context.handle(
        _historyLimitMeta,
        historyLimit.isAcceptableOrUnknown(
          data['history_limit']!,
          _historyLimitMeta,
        ),
      );
    }
    if (data.containsKey('launch_at_startup')) {
      context.handle(
        _launchAtStartupMeta,
        launchAtStartup.isAcceptableOrUnknown(
          data['launch_at_startup']!,
          _launchAtStartupMeta,
        ),
      );
    }
    if (data.containsKey('hotkey_json')) {
      context.handle(
        _hotkeyJsonMeta,
        hotkeyJson.isAcceptableOrUnknown(data['hotkey_json']!, _hotkeyJsonMeta),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  AppSetting map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return AppSetting(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      historyLimit: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}history_limit'],
      )!,
      launchAtStartup: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}launch_at_startup'],
      )!,
      hotkeyJson: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}hotkey_json'],
      ),
    );
  }

  @override
  $AppSettingsTable createAlias(String alias) {
    return $AppSettingsTable(attachedDatabase, alias);
  }
}

class AppSetting extends DataClass implements Insertable<AppSetting> {
  final int id;
  final int historyLimit;
  final bool launchAtStartup;
  final String? hotkeyJson;
  const AppSetting({
    required this.id,
    required this.historyLimit,
    required this.launchAtStartup,
    this.hotkeyJson,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['history_limit'] = Variable<int>(historyLimit);
    map['launch_at_startup'] = Variable<bool>(launchAtStartup);
    if (!nullToAbsent || hotkeyJson != null) {
      map['hotkey_json'] = Variable<String>(hotkeyJson);
    }
    return map;
  }

  AppSettingsCompanion toCompanion(bool nullToAbsent) {
    return AppSettingsCompanion(
      id: Value(id),
      historyLimit: Value(historyLimit),
      launchAtStartup: Value(launchAtStartup),
      hotkeyJson: hotkeyJson == null && nullToAbsent
          ? const Value.absent()
          : Value(hotkeyJson),
    );
  }

  factory AppSetting.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return AppSetting(
      id: serializer.fromJson<int>(json['id']),
      historyLimit: serializer.fromJson<int>(json['historyLimit']),
      launchAtStartup: serializer.fromJson<bool>(json['launchAtStartup']),
      hotkeyJson: serializer.fromJson<String?>(json['hotkeyJson']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'historyLimit': serializer.toJson<int>(historyLimit),
      'launchAtStartup': serializer.toJson<bool>(launchAtStartup),
      'hotkeyJson': serializer.toJson<String?>(hotkeyJson),
    };
  }

  AppSetting copyWith({
    int? id,
    int? historyLimit,
    bool? launchAtStartup,
    Value<String?> hotkeyJson = const Value.absent(),
  }) => AppSetting(
    id: id ?? this.id,
    historyLimit: historyLimit ?? this.historyLimit,
    launchAtStartup: launchAtStartup ?? this.launchAtStartup,
    hotkeyJson: hotkeyJson.present ? hotkeyJson.value : this.hotkeyJson,
  );
  AppSetting copyWithCompanion(AppSettingsCompanion data) {
    return AppSetting(
      id: data.id.present ? data.id.value : this.id,
      historyLimit: data.historyLimit.present
          ? data.historyLimit.value
          : this.historyLimit,
      launchAtStartup: data.launchAtStartup.present
          ? data.launchAtStartup.value
          : this.launchAtStartup,
      hotkeyJson: data.hotkeyJson.present
          ? data.hotkeyJson.value
          : this.hotkeyJson,
    );
  }

  @override
  String toString() {
    return (StringBuffer('AppSetting(')
          ..write('id: $id, ')
          ..write('historyLimit: $historyLimit, ')
          ..write('launchAtStartup: $launchAtStartup, ')
          ..write('hotkeyJson: $hotkeyJson')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode =>
      Object.hash(id, historyLimit, launchAtStartup, hotkeyJson);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is AppSetting &&
          other.id == this.id &&
          other.historyLimit == this.historyLimit &&
          other.launchAtStartup == this.launchAtStartup &&
          other.hotkeyJson == this.hotkeyJson);
}

class AppSettingsCompanion extends UpdateCompanion<AppSetting> {
  final Value<int> id;
  final Value<int> historyLimit;
  final Value<bool> launchAtStartup;
  final Value<String?> hotkeyJson;
  const AppSettingsCompanion({
    this.id = const Value.absent(),
    this.historyLimit = const Value.absent(),
    this.launchAtStartup = const Value.absent(),
    this.hotkeyJson = const Value.absent(),
  });
  AppSettingsCompanion.insert({
    this.id = const Value.absent(),
    this.historyLimit = const Value.absent(),
    this.launchAtStartup = const Value.absent(),
    this.hotkeyJson = const Value.absent(),
  });
  static Insertable<AppSetting> custom({
    Expression<int>? id,
    Expression<int>? historyLimit,
    Expression<bool>? launchAtStartup,
    Expression<String>? hotkeyJson,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (historyLimit != null) 'history_limit': historyLimit,
      if (launchAtStartup != null) 'launch_at_startup': launchAtStartup,
      if (hotkeyJson != null) 'hotkey_json': hotkeyJson,
    });
  }

  AppSettingsCompanion copyWith({
    Value<int>? id,
    Value<int>? historyLimit,
    Value<bool>? launchAtStartup,
    Value<String?>? hotkeyJson,
  }) {
    return AppSettingsCompanion(
      id: id ?? this.id,
      historyLimit: historyLimit ?? this.historyLimit,
      launchAtStartup: launchAtStartup ?? this.launchAtStartup,
      hotkeyJson: hotkeyJson ?? this.hotkeyJson,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (historyLimit.present) {
      map['history_limit'] = Variable<int>(historyLimit.value);
    }
    if (launchAtStartup.present) {
      map['launch_at_startup'] = Variable<bool>(launchAtStartup.value);
    }
    if (hotkeyJson.present) {
      map['hotkey_json'] = Variable<String>(hotkeyJson.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('AppSettingsCompanion(')
          ..write('id: $id, ')
          ..write('historyLimit: $historyLimit, ')
          ..write('launchAtStartup: $launchAtStartup, ')
          ..write('hotkeyJson: $hotkeyJson')
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
  late final $AppSettingsTable appSettings = $AppSettingsTable(this);
  @override
  Iterable<TableInfo<Table, Object?>> get allTables =>
      allSchemaEntities.whereType<TableInfo<Table, Object?>>();
  @override
  List<DatabaseSchemaEntity> get allSchemaEntities => [
    clipboardEntries,
    appSettings,
  ];
}

typedef $$ClipboardEntriesTableCreateCompanionBuilder =
    ClipboardEntriesCompanion Function({
      Value<int> id,
      required String content,
      Value<String> type,
      Value<DateTime> createdAt,
      Value<bool> isPinned,
    });
typedef $$ClipboardEntriesTableUpdateCompanionBuilder =
    ClipboardEntriesCompanion Function({
      Value<int> id,
      Value<String> content,
      Value<String> type,
      Value<DateTime> createdAt,
      Value<bool> isPinned,
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
              }) => ClipboardEntriesCompanion(
                id: id,
                content: content,
                type: type,
                createdAt: createdAt,
                isPinned: isPinned,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required String content,
                Value<String> type = const Value.absent(),
                Value<DateTime> createdAt = const Value.absent(),
                Value<bool> isPinned = const Value.absent(),
              }) => ClipboardEntriesCompanion.insert(
                id: id,
                content: content,
                type: type,
                createdAt: createdAt,
                isPinned: isPinned,
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
typedef $$AppSettingsTableCreateCompanionBuilder =
    AppSettingsCompanion Function({
      Value<int> id,
      Value<int> historyLimit,
      Value<bool> launchAtStartup,
      Value<String?> hotkeyJson,
    });
typedef $$AppSettingsTableUpdateCompanionBuilder =
    AppSettingsCompanion Function({
      Value<int> id,
      Value<int> historyLimit,
      Value<bool> launchAtStartup,
      Value<String?> hotkeyJson,
    });

class $$AppSettingsTableFilterComposer
    extends Composer<_$AppDatabase, $AppSettingsTable> {
  $$AppSettingsTableFilterComposer({
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

  ColumnFilters<int> get historyLimit => $composableBuilder(
    column: $table.historyLimit,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get launchAtStartup => $composableBuilder(
    column: $table.launchAtStartup,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get hotkeyJson => $composableBuilder(
    column: $table.hotkeyJson,
    builder: (column) => ColumnFilters(column),
  );
}

class $$AppSettingsTableOrderingComposer
    extends Composer<_$AppDatabase, $AppSettingsTable> {
  $$AppSettingsTableOrderingComposer({
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

  ColumnOrderings<int> get historyLimit => $composableBuilder(
    column: $table.historyLimit,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get launchAtStartup => $composableBuilder(
    column: $table.launchAtStartup,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get hotkeyJson => $composableBuilder(
    column: $table.hotkeyJson,
    builder: (column) => ColumnOrderings(column),
  );
}

class $$AppSettingsTableAnnotationComposer
    extends Composer<_$AppDatabase, $AppSettingsTable> {
  $$AppSettingsTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  GeneratedColumn<int> get id =>
      $composableBuilder(column: $table.id, builder: (column) => column);

  GeneratedColumn<int> get historyLimit => $composableBuilder(
    column: $table.historyLimit,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get launchAtStartup => $composableBuilder(
    column: $table.launchAtStartup,
    builder: (column) => column,
  );

  GeneratedColumn<String> get hotkeyJson => $composableBuilder(
    column: $table.hotkeyJson,
    builder: (column) => column,
  );
}

class $$AppSettingsTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $AppSettingsTable,
          AppSetting,
          $$AppSettingsTableFilterComposer,
          $$AppSettingsTableOrderingComposer,
          $$AppSettingsTableAnnotationComposer,
          $$AppSettingsTableCreateCompanionBuilder,
          $$AppSettingsTableUpdateCompanionBuilder,
          (
            AppSetting,
            BaseReferences<_$AppDatabase, $AppSettingsTable, AppSetting>,
          ),
          AppSetting,
          PrefetchHooks Function()
        > {
  $$AppSettingsTableTableManager(_$AppDatabase db, $AppSettingsTable table)
    : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$AppSettingsTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$AppSettingsTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$AppSettingsTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<int> historyLimit = const Value.absent(),
                Value<bool> launchAtStartup = const Value.absent(),
                Value<String?> hotkeyJson = const Value.absent(),
              }) => AppSettingsCompanion(
                id: id,
                historyLimit: historyLimit,
                launchAtStartup: launchAtStartup,
                hotkeyJson: hotkeyJson,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<int> historyLimit = const Value.absent(),
                Value<bool> launchAtStartup = const Value.absent(),
                Value<String?> hotkeyJson = const Value.absent(),
              }) => AppSettingsCompanion.insert(
                id: id,
                historyLimit: historyLimit,
                launchAtStartup: launchAtStartup,
                hotkeyJson: hotkeyJson,
              ),
          withReferenceMapper: (p0) => p0
              .map((e) => (e.readTable(table), BaseReferences(db, table, e)))
              .toList(),
          prefetchHooksCallback: null,
        ),
      );
}

typedef $$AppSettingsTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $AppSettingsTable,
      AppSetting,
      $$AppSettingsTableFilterComposer,
      $$AppSettingsTableOrderingComposer,
      $$AppSettingsTableAnnotationComposer,
      $$AppSettingsTableCreateCompanionBuilder,
      $$AppSettingsTableUpdateCompanionBuilder,
      (
        AppSetting,
        BaseReferences<_$AppDatabase, $AppSettingsTable, AppSetting>,
      ),
      AppSetting,
      PrefetchHooks Function()
    >;

class $AppDatabaseManager {
  final _$AppDatabase _db;
  $AppDatabaseManager(this._db);
  $$ClipboardEntriesTableTableManager get clipboardEntries =>
      $$ClipboardEntriesTableTableManager(_db, _db.clipboardEntries);
  $$AppSettingsTableTableManager get appSettings =>
      $$AppSettingsTableTableManager(_db, _db.appSettings);
}
