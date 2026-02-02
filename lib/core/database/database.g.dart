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
  static const VerificationMeta _autoCheckUpdatesMeta = const VerificationMeta(
    'autoCheckUpdates',
  );
  @override
  late final GeneratedColumn<bool> autoCheckUpdates = GeneratedColumn<bool>(
    'auto_check_updates',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("auto_check_updates" IN (0, 1))',
    ),
    defaultValue: const Constant(true),
  );
  static const VerificationMeta _hotkeyOpenMeta = const VerificationMeta(
    'hotkeyOpen',
  );
  @override
  late final GeneratedColumn<String> hotkeyOpen = GeneratedColumn<String>(
    'hotkey_open',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('Alt+V'),
  );
  static const VerificationMeta _hotkeyPinMeta = const VerificationMeta(
    'hotkeyPin',
  );
  @override
  late final GeneratedColumn<String> hotkeyPin = GeneratedColumn<String>(
    'hotkey_pin',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant(''),
  );
  static const VerificationMeta _hotkeyDeleteMeta = const VerificationMeta(
    'hotkeyDelete',
  );
  @override
  late final GeneratedColumn<String> hotkeyDelete = GeneratedColumn<String>(
    'hotkey_delete',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant(''),
  );
  static const VerificationMeta _autoPasteMeta = const VerificationMeta(
    'autoPaste',
  );
  @override
  late final GeneratedColumn<bool> autoPaste = GeneratedColumn<bool>(
    'auto_paste',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("auto_paste" IN (0, 1))',
    ),
    defaultValue: const Constant(true),
  );
  static const VerificationMeta _pastePlainMeta = const VerificationMeta(
    'pastePlain',
  );
  @override
  late final GeneratedColumn<bool> pastePlain = GeneratedColumn<bool>(
    'paste_plain',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("paste_plain" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _searchModeMeta = const VerificationMeta(
    'searchMode',
  );
  @override
  late final GeneratedColumn<String> searchMode = GeneratedColumn<String>(
    'search_mode',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('fuzzy'),
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
  static const VerificationMeta _saveTextMeta = const VerificationMeta(
    'saveText',
  );
  @override
  late final GeneratedColumn<bool> saveText = GeneratedColumn<bool>(
    'save_text',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("save_text" IN (0, 1))',
    ),
    defaultValue: const Constant(true),
  );
  static const VerificationMeta _saveImagesMeta = const VerificationMeta(
    'saveImages',
  );
  @override
  late final GeneratedColumn<bool> saveImages = GeneratedColumn<bool>(
    'save_images',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("save_images" IN (0, 1))',
    ),
    defaultValue: const Constant(true),
  );
  static const VerificationMeta _saveFilesMeta = const VerificationMeta(
    'saveFiles',
  );
  @override
  late final GeneratedColumn<bool> saveFiles = GeneratedColumn<bool>(
    'save_files',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("save_files" IN (0, 1))',
    ),
    defaultValue: const Constant(true),
  );
  static const VerificationMeta _popupPositionMeta = const VerificationMeta(
    'popupPosition',
  );
  @override
  late final GeneratedColumn<String> popupPosition = GeneratedColumn<String>(
    'popup_position',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('cursor'),
  );
  static const VerificationMeta _pinPositionMeta = const VerificationMeta(
    'pinPosition',
  );
  @override
  late final GeneratedColumn<String> pinPosition = GeneratedColumn<String>(
    'pin_position',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('top'),
  );
  static const VerificationMeta _imageHeightMeta = const VerificationMeta(
    'imageHeight',
  );
  @override
  late final GeneratedColumn<int> imageHeight = GeneratedColumn<int>(
    'image_height',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(40),
  );
  static const VerificationMeta _previewDelayMeta = const VerificationMeta(
    'previewDelay',
  );
  @override
  late final GeneratedColumn<int> previewDelay = GeneratedColumn<int>(
    'preview_delay',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(1500),
  );
  static const VerificationMeta _highlightMatchMeta = const VerificationMeta(
    'highlightMatch',
  );
  @override
  late final GeneratedColumn<String> highlightMatch = GeneratedColumn<String>(
    'highlight_match',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('bold'),
  );
  static const VerificationMeta _showSpecialCharsMeta = const VerificationMeta(
    'showSpecialChars',
  );
  @override
  late final GeneratedColumn<bool> showSpecialChars = GeneratedColumn<bool>(
    'show_special_chars',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("show_special_chars" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _showMenuBarIconMeta = const VerificationMeta(
    'showMenuBarIcon',
  );
  @override
  late final GeneratedColumn<bool> showMenuBarIcon = GeneratedColumn<bool>(
    'show_menu_bar_icon',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("show_menu_bar_icon" IN (0, 1))',
    ),
    defaultValue: const Constant(true),
  );
  static const VerificationMeta _menuBarIconTypeMeta = const VerificationMeta(
    'menuBarIconType',
  );
  @override
  late final GeneratedColumn<String> menuBarIconType = GeneratedColumn<String>(
    'menu_bar_icon_type',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('clipboard'),
  );
  static const VerificationMeta _showClipboardNearIconMeta =
      const VerificationMeta('showClipboardNearIcon');
  @override
  late final GeneratedColumn<bool> showClipboardNearIcon =
      GeneratedColumn<bool>(
        'show_clipboard_near_icon',
        aliasedName,
        false,
        type: DriftSqlType.bool,
        requiredDuringInsert: false,
        defaultConstraints: GeneratedColumn.constraintIsAlways(
          'CHECK ("show_clipboard_near_icon" IN (0, 1))',
        ),
        defaultValue: const Constant(false),
      );
  static const VerificationMeta _showSearchBoxMeta = const VerificationMeta(
    'showSearchBox',
  );
  @override
  late final GeneratedColumn<String> showSearchBox = GeneratedColumn<String>(
    'show_search_box',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('always'),
  );
  static const VerificationMeta _showAppNameMeta = const VerificationMeta(
    'showAppName',
  );
  @override
  late final GeneratedColumn<bool> showAppName = GeneratedColumn<bool>(
    'show_app_name',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("show_app_name" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _showAppIconMeta = const VerificationMeta(
    'showAppIcon',
  );
  @override
  late final GeneratedColumn<bool> showAppIcon = GeneratedColumn<bool>(
    'show_app_icon',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("show_app_icon" IN (0, 1))',
    ),
    defaultValue: const Constant(true),
  );
  static const VerificationMeta _showFooterMenuMeta = const VerificationMeta(
    'showFooterMenu',
  );
  @override
  late final GeneratedColumn<bool> showFooterMenu = GeneratedColumn<bool>(
    'show_footer_menu',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("show_footer_menu" IN (0, 1))',
    ),
    defaultValue: const Constant(true),
  );
  static const VerificationMeta _windowWidthMeta = const VerificationMeta(
    'windowWidth',
  );
  @override
  late final GeneratedColumn<double> windowWidth = GeneratedColumn<double>(
    'window_width',
    aliasedName,
    false,
    type: DriftSqlType.double,
    requiredDuringInsert: false,
    defaultValue: const Constant(350.0),
  );
  static const VerificationMeta _themeModeMeta = const VerificationMeta(
    'themeMode',
  );
  @override
  late final GeneratedColumn<String> themeMode = GeneratedColumn<String>(
    'theme_mode',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('system'),
  );
  static const VerificationMeta _isPausedMeta = const VerificationMeta(
    'isPaused',
  );
  @override
  late final GeneratedColumn<bool> isPaused = GeneratedColumn<bool>(
    'is_paused',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("is_paused" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _clearOnExitMeta = const VerificationMeta(
    'clearOnExit',
  );
  @override
  late final GeneratedColumn<bool> clearOnExit = GeneratedColumn<bool>(
    'clear_on_exit',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("clear_on_exit" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _clearSystemClipboardMeta =
      const VerificationMeta('clearSystemClipboard');
  @override
  late final GeneratedColumn<bool> clearSystemClipboard = GeneratedColumn<bool>(
    'clear_system_clipboard',
    aliasedName,
    false,
    type: DriftSqlType.bool,
    requiredDuringInsert: false,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'CHECK ("clear_system_clipboard" IN (0, 1))',
    ),
    defaultValue: const Constant(false),
  );
  static const VerificationMeta _ignoreAppsJsonMeta = const VerificationMeta(
    'ignoreAppsJson',
  );
  @override
  late final GeneratedColumn<String> ignoreAppsJson = GeneratedColumn<String>(
    'ignore_apps_json',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
    defaultValue: const Constant('[]'),
  );
  @override
  List<GeneratedColumn> get $columns => [
    id,
    launchAtStartup,
    autoCheckUpdates,
    hotkeyOpen,
    hotkeyPin,
    hotkeyDelete,
    autoPaste,
    pastePlain,
    searchMode,
    historyLimit,
    saveText,
    saveImages,
    saveFiles,
    popupPosition,
    pinPosition,
    imageHeight,
    previewDelay,
    highlightMatch,
    showSpecialChars,
    showMenuBarIcon,
    menuBarIconType,
    showClipboardNearIcon,
    showSearchBox,
    showAppName,
    showAppIcon,
    showFooterMenu,
    windowWidth,
    themeMode,
    isPaused,
    clearOnExit,
    clearSystemClipboard,
    ignoreAppsJson,
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
    if (data.containsKey('launch_at_startup')) {
      context.handle(
        _launchAtStartupMeta,
        launchAtStartup.isAcceptableOrUnknown(
          data['launch_at_startup']!,
          _launchAtStartupMeta,
        ),
      );
    }
    if (data.containsKey('auto_check_updates')) {
      context.handle(
        _autoCheckUpdatesMeta,
        autoCheckUpdates.isAcceptableOrUnknown(
          data['auto_check_updates']!,
          _autoCheckUpdatesMeta,
        ),
      );
    }
    if (data.containsKey('hotkey_open')) {
      context.handle(
        _hotkeyOpenMeta,
        hotkeyOpen.isAcceptableOrUnknown(data['hotkey_open']!, _hotkeyOpenMeta),
      );
    }
    if (data.containsKey('hotkey_pin')) {
      context.handle(
        _hotkeyPinMeta,
        hotkeyPin.isAcceptableOrUnknown(data['hotkey_pin']!, _hotkeyPinMeta),
      );
    }
    if (data.containsKey('hotkey_delete')) {
      context.handle(
        _hotkeyDeleteMeta,
        hotkeyDelete.isAcceptableOrUnknown(
          data['hotkey_delete']!,
          _hotkeyDeleteMeta,
        ),
      );
    }
    if (data.containsKey('auto_paste')) {
      context.handle(
        _autoPasteMeta,
        autoPaste.isAcceptableOrUnknown(data['auto_paste']!, _autoPasteMeta),
      );
    }
    if (data.containsKey('paste_plain')) {
      context.handle(
        _pastePlainMeta,
        pastePlain.isAcceptableOrUnknown(data['paste_plain']!, _pastePlainMeta),
      );
    }
    if (data.containsKey('search_mode')) {
      context.handle(
        _searchModeMeta,
        searchMode.isAcceptableOrUnknown(data['search_mode']!, _searchModeMeta),
      );
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
    if (data.containsKey('save_text')) {
      context.handle(
        _saveTextMeta,
        saveText.isAcceptableOrUnknown(data['save_text']!, _saveTextMeta),
      );
    }
    if (data.containsKey('save_images')) {
      context.handle(
        _saveImagesMeta,
        saveImages.isAcceptableOrUnknown(data['save_images']!, _saveImagesMeta),
      );
    }
    if (data.containsKey('save_files')) {
      context.handle(
        _saveFilesMeta,
        saveFiles.isAcceptableOrUnknown(data['save_files']!, _saveFilesMeta),
      );
    }
    if (data.containsKey('popup_position')) {
      context.handle(
        _popupPositionMeta,
        popupPosition.isAcceptableOrUnknown(
          data['popup_position']!,
          _popupPositionMeta,
        ),
      );
    }
    if (data.containsKey('pin_position')) {
      context.handle(
        _pinPositionMeta,
        pinPosition.isAcceptableOrUnknown(
          data['pin_position']!,
          _pinPositionMeta,
        ),
      );
    }
    if (data.containsKey('image_height')) {
      context.handle(
        _imageHeightMeta,
        imageHeight.isAcceptableOrUnknown(
          data['image_height']!,
          _imageHeightMeta,
        ),
      );
    }
    if (data.containsKey('preview_delay')) {
      context.handle(
        _previewDelayMeta,
        previewDelay.isAcceptableOrUnknown(
          data['preview_delay']!,
          _previewDelayMeta,
        ),
      );
    }
    if (data.containsKey('highlight_match')) {
      context.handle(
        _highlightMatchMeta,
        highlightMatch.isAcceptableOrUnknown(
          data['highlight_match']!,
          _highlightMatchMeta,
        ),
      );
    }
    if (data.containsKey('show_special_chars')) {
      context.handle(
        _showSpecialCharsMeta,
        showSpecialChars.isAcceptableOrUnknown(
          data['show_special_chars']!,
          _showSpecialCharsMeta,
        ),
      );
    }
    if (data.containsKey('show_menu_bar_icon')) {
      context.handle(
        _showMenuBarIconMeta,
        showMenuBarIcon.isAcceptableOrUnknown(
          data['show_menu_bar_icon']!,
          _showMenuBarIconMeta,
        ),
      );
    }
    if (data.containsKey('menu_bar_icon_type')) {
      context.handle(
        _menuBarIconTypeMeta,
        menuBarIconType.isAcceptableOrUnknown(
          data['menu_bar_icon_type']!,
          _menuBarIconTypeMeta,
        ),
      );
    }
    if (data.containsKey('show_clipboard_near_icon')) {
      context.handle(
        _showClipboardNearIconMeta,
        showClipboardNearIcon.isAcceptableOrUnknown(
          data['show_clipboard_near_icon']!,
          _showClipboardNearIconMeta,
        ),
      );
    }
    if (data.containsKey('show_search_box')) {
      context.handle(
        _showSearchBoxMeta,
        showSearchBox.isAcceptableOrUnknown(
          data['show_search_box']!,
          _showSearchBoxMeta,
        ),
      );
    }
    if (data.containsKey('show_app_name')) {
      context.handle(
        _showAppNameMeta,
        showAppName.isAcceptableOrUnknown(
          data['show_app_name']!,
          _showAppNameMeta,
        ),
      );
    }
    if (data.containsKey('show_app_icon')) {
      context.handle(
        _showAppIconMeta,
        showAppIcon.isAcceptableOrUnknown(
          data['show_app_icon']!,
          _showAppIconMeta,
        ),
      );
    }
    if (data.containsKey('show_footer_menu')) {
      context.handle(
        _showFooterMenuMeta,
        showFooterMenu.isAcceptableOrUnknown(
          data['show_footer_menu']!,
          _showFooterMenuMeta,
        ),
      );
    }
    if (data.containsKey('window_width')) {
      context.handle(
        _windowWidthMeta,
        windowWidth.isAcceptableOrUnknown(
          data['window_width']!,
          _windowWidthMeta,
        ),
      );
    }
    if (data.containsKey('theme_mode')) {
      context.handle(
        _themeModeMeta,
        themeMode.isAcceptableOrUnknown(data['theme_mode']!, _themeModeMeta),
      );
    }
    if (data.containsKey('is_paused')) {
      context.handle(
        _isPausedMeta,
        isPaused.isAcceptableOrUnknown(data['is_paused']!, _isPausedMeta),
      );
    }
    if (data.containsKey('clear_on_exit')) {
      context.handle(
        _clearOnExitMeta,
        clearOnExit.isAcceptableOrUnknown(
          data['clear_on_exit']!,
          _clearOnExitMeta,
        ),
      );
    }
    if (data.containsKey('clear_system_clipboard')) {
      context.handle(
        _clearSystemClipboardMeta,
        clearSystemClipboard.isAcceptableOrUnknown(
          data['clear_system_clipboard']!,
          _clearSystemClipboardMeta,
        ),
      );
    }
    if (data.containsKey('ignore_apps_json')) {
      context.handle(
        _ignoreAppsJsonMeta,
        ignoreAppsJson.isAcceptableOrUnknown(
          data['ignore_apps_json']!,
          _ignoreAppsJsonMeta,
        ),
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
      launchAtStartup: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}launch_at_startup'],
      )!,
      autoCheckUpdates: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}auto_check_updates'],
      )!,
      hotkeyOpen: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}hotkey_open'],
      )!,
      hotkeyPin: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}hotkey_pin'],
      )!,
      hotkeyDelete: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}hotkey_delete'],
      )!,
      autoPaste: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}auto_paste'],
      )!,
      pastePlain: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}paste_plain'],
      )!,
      searchMode: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}search_mode'],
      )!,
      historyLimit: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}history_limit'],
      )!,
      saveText: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}save_text'],
      )!,
      saveImages: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}save_images'],
      )!,
      saveFiles: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}save_files'],
      )!,
      popupPosition: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}popup_position'],
      )!,
      pinPosition: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}pin_position'],
      )!,
      imageHeight: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}image_height'],
      )!,
      previewDelay: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}preview_delay'],
      )!,
      highlightMatch: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}highlight_match'],
      )!,
      showSpecialChars: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}show_special_chars'],
      )!,
      showMenuBarIcon: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}show_menu_bar_icon'],
      )!,
      menuBarIconType: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}menu_bar_icon_type'],
      )!,
      showClipboardNearIcon: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}show_clipboard_near_icon'],
      )!,
      showSearchBox: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}show_search_box'],
      )!,
      showAppName: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}show_app_name'],
      )!,
      showAppIcon: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}show_app_icon'],
      )!,
      showFooterMenu: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}show_footer_menu'],
      )!,
      windowWidth: attachedDatabase.typeMapping.read(
        DriftSqlType.double,
        data['${effectivePrefix}window_width'],
      )!,
      themeMode: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}theme_mode'],
      )!,
      isPaused: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}is_paused'],
      )!,
      clearOnExit: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}clear_on_exit'],
      )!,
      clearSystemClipboard: attachedDatabase.typeMapping.read(
        DriftSqlType.bool,
        data['${effectivePrefix}clear_system_clipboard'],
      )!,
      ignoreAppsJson: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}ignore_apps_json'],
      )!,
    );
  }

  @override
  $AppSettingsTable createAlias(String alias) {
    return $AppSettingsTable(attachedDatabase, alias);
  }
}

class AppSetting extends DataClass implements Insertable<AppSetting> {
  final int id;
  final bool launchAtStartup;
  final bool autoCheckUpdates;
  final String hotkeyOpen;
  final String hotkeyPin;
  final String hotkeyDelete;
  final bool autoPaste;
  final bool pastePlain;
  final String searchMode;
  final int historyLimit;
  final bool saveText;
  final bool saveImages;
  final bool saveFiles;
  final String popupPosition;
  final String pinPosition;
  final int imageHeight;
  final int previewDelay;
  final String highlightMatch;
  final bool showSpecialChars;
  final bool showMenuBarIcon;
  final String menuBarIconType;
  final bool showClipboardNearIcon;
  final String showSearchBox;
  final bool showAppName;
  final bool showAppIcon;
  final bool showFooterMenu;
  final double windowWidth;
  final String themeMode;
  final bool isPaused;
  final bool clearOnExit;
  final bool clearSystemClipboard;
  final String ignoreAppsJson;
  const AppSetting({
    required this.id,
    required this.launchAtStartup,
    required this.autoCheckUpdates,
    required this.hotkeyOpen,
    required this.hotkeyPin,
    required this.hotkeyDelete,
    required this.autoPaste,
    required this.pastePlain,
    required this.searchMode,
    required this.historyLimit,
    required this.saveText,
    required this.saveImages,
    required this.saveFiles,
    required this.popupPosition,
    required this.pinPosition,
    required this.imageHeight,
    required this.previewDelay,
    required this.highlightMatch,
    required this.showSpecialChars,
    required this.showMenuBarIcon,
    required this.menuBarIconType,
    required this.showClipboardNearIcon,
    required this.showSearchBox,
    required this.showAppName,
    required this.showAppIcon,
    required this.showFooterMenu,
    required this.windowWidth,
    required this.themeMode,
    required this.isPaused,
    required this.clearOnExit,
    required this.clearSystemClipboard,
    required this.ignoreAppsJson,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['launch_at_startup'] = Variable<bool>(launchAtStartup);
    map['auto_check_updates'] = Variable<bool>(autoCheckUpdates);
    map['hotkey_open'] = Variable<String>(hotkeyOpen);
    map['hotkey_pin'] = Variable<String>(hotkeyPin);
    map['hotkey_delete'] = Variable<String>(hotkeyDelete);
    map['auto_paste'] = Variable<bool>(autoPaste);
    map['paste_plain'] = Variable<bool>(pastePlain);
    map['search_mode'] = Variable<String>(searchMode);
    map['history_limit'] = Variable<int>(historyLimit);
    map['save_text'] = Variable<bool>(saveText);
    map['save_images'] = Variable<bool>(saveImages);
    map['save_files'] = Variable<bool>(saveFiles);
    map['popup_position'] = Variable<String>(popupPosition);
    map['pin_position'] = Variable<String>(pinPosition);
    map['image_height'] = Variable<int>(imageHeight);
    map['preview_delay'] = Variable<int>(previewDelay);
    map['highlight_match'] = Variable<String>(highlightMatch);
    map['show_special_chars'] = Variable<bool>(showSpecialChars);
    map['show_menu_bar_icon'] = Variable<bool>(showMenuBarIcon);
    map['menu_bar_icon_type'] = Variable<String>(menuBarIconType);
    map['show_clipboard_near_icon'] = Variable<bool>(showClipboardNearIcon);
    map['show_search_box'] = Variable<String>(showSearchBox);
    map['show_app_name'] = Variable<bool>(showAppName);
    map['show_app_icon'] = Variable<bool>(showAppIcon);
    map['show_footer_menu'] = Variable<bool>(showFooterMenu);
    map['window_width'] = Variable<double>(windowWidth);
    map['theme_mode'] = Variable<String>(themeMode);
    map['is_paused'] = Variable<bool>(isPaused);
    map['clear_on_exit'] = Variable<bool>(clearOnExit);
    map['clear_system_clipboard'] = Variable<bool>(clearSystemClipboard);
    map['ignore_apps_json'] = Variable<String>(ignoreAppsJson);
    return map;
  }

  AppSettingsCompanion toCompanion(bool nullToAbsent) {
    return AppSettingsCompanion(
      id: Value(id),
      launchAtStartup: Value(launchAtStartup),
      autoCheckUpdates: Value(autoCheckUpdates),
      hotkeyOpen: Value(hotkeyOpen),
      hotkeyPin: Value(hotkeyPin),
      hotkeyDelete: Value(hotkeyDelete),
      autoPaste: Value(autoPaste),
      pastePlain: Value(pastePlain),
      searchMode: Value(searchMode),
      historyLimit: Value(historyLimit),
      saveText: Value(saveText),
      saveImages: Value(saveImages),
      saveFiles: Value(saveFiles),
      popupPosition: Value(popupPosition),
      pinPosition: Value(pinPosition),
      imageHeight: Value(imageHeight),
      previewDelay: Value(previewDelay),
      highlightMatch: Value(highlightMatch),
      showSpecialChars: Value(showSpecialChars),
      showMenuBarIcon: Value(showMenuBarIcon),
      menuBarIconType: Value(menuBarIconType),
      showClipboardNearIcon: Value(showClipboardNearIcon),
      showSearchBox: Value(showSearchBox),
      showAppName: Value(showAppName),
      showAppIcon: Value(showAppIcon),
      showFooterMenu: Value(showFooterMenu),
      windowWidth: Value(windowWidth),
      themeMode: Value(themeMode),
      isPaused: Value(isPaused),
      clearOnExit: Value(clearOnExit),
      clearSystemClipboard: Value(clearSystemClipboard),
      ignoreAppsJson: Value(ignoreAppsJson),
    );
  }

  factory AppSetting.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return AppSetting(
      id: serializer.fromJson<int>(json['id']),
      launchAtStartup: serializer.fromJson<bool>(json['launchAtStartup']),
      autoCheckUpdates: serializer.fromJson<bool>(json['autoCheckUpdates']),
      hotkeyOpen: serializer.fromJson<String>(json['hotkeyOpen']),
      hotkeyPin: serializer.fromJson<String>(json['hotkeyPin']),
      hotkeyDelete: serializer.fromJson<String>(json['hotkeyDelete']),
      autoPaste: serializer.fromJson<bool>(json['autoPaste']),
      pastePlain: serializer.fromJson<bool>(json['pastePlain']),
      searchMode: serializer.fromJson<String>(json['searchMode']),
      historyLimit: serializer.fromJson<int>(json['historyLimit']),
      saveText: serializer.fromJson<bool>(json['saveText']),
      saveImages: serializer.fromJson<bool>(json['saveImages']),
      saveFiles: serializer.fromJson<bool>(json['saveFiles']),
      popupPosition: serializer.fromJson<String>(json['popupPosition']),
      pinPosition: serializer.fromJson<String>(json['pinPosition']),
      imageHeight: serializer.fromJson<int>(json['imageHeight']),
      previewDelay: serializer.fromJson<int>(json['previewDelay']),
      highlightMatch: serializer.fromJson<String>(json['highlightMatch']),
      showSpecialChars: serializer.fromJson<bool>(json['showSpecialChars']),
      showMenuBarIcon: serializer.fromJson<bool>(json['showMenuBarIcon']),
      menuBarIconType: serializer.fromJson<String>(json['menuBarIconType']),
      showClipboardNearIcon: serializer.fromJson<bool>(
        json['showClipboardNearIcon'],
      ),
      showSearchBox: serializer.fromJson<String>(json['showSearchBox']),
      showAppName: serializer.fromJson<bool>(json['showAppName']),
      showAppIcon: serializer.fromJson<bool>(json['showAppIcon']),
      showFooterMenu: serializer.fromJson<bool>(json['showFooterMenu']),
      windowWidth: serializer.fromJson<double>(json['windowWidth']),
      themeMode: serializer.fromJson<String>(json['themeMode']),
      isPaused: serializer.fromJson<bool>(json['isPaused']),
      clearOnExit: serializer.fromJson<bool>(json['clearOnExit']),
      clearSystemClipboard: serializer.fromJson<bool>(
        json['clearSystemClipboard'],
      ),
      ignoreAppsJson: serializer.fromJson<String>(json['ignoreAppsJson']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'launchAtStartup': serializer.toJson<bool>(launchAtStartup),
      'autoCheckUpdates': serializer.toJson<bool>(autoCheckUpdates),
      'hotkeyOpen': serializer.toJson<String>(hotkeyOpen),
      'hotkeyPin': serializer.toJson<String>(hotkeyPin),
      'hotkeyDelete': serializer.toJson<String>(hotkeyDelete),
      'autoPaste': serializer.toJson<bool>(autoPaste),
      'pastePlain': serializer.toJson<bool>(pastePlain),
      'searchMode': serializer.toJson<String>(searchMode),
      'historyLimit': serializer.toJson<int>(historyLimit),
      'saveText': serializer.toJson<bool>(saveText),
      'saveImages': serializer.toJson<bool>(saveImages),
      'saveFiles': serializer.toJson<bool>(saveFiles),
      'popupPosition': serializer.toJson<String>(popupPosition),
      'pinPosition': serializer.toJson<String>(pinPosition),
      'imageHeight': serializer.toJson<int>(imageHeight),
      'previewDelay': serializer.toJson<int>(previewDelay),
      'highlightMatch': serializer.toJson<String>(highlightMatch),
      'showSpecialChars': serializer.toJson<bool>(showSpecialChars),
      'showMenuBarIcon': serializer.toJson<bool>(showMenuBarIcon),
      'menuBarIconType': serializer.toJson<String>(menuBarIconType),
      'showClipboardNearIcon': serializer.toJson<bool>(showClipboardNearIcon),
      'showSearchBox': serializer.toJson<String>(showSearchBox),
      'showAppName': serializer.toJson<bool>(showAppName),
      'showAppIcon': serializer.toJson<bool>(showAppIcon),
      'showFooterMenu': serializer.toJson<bool>(showFooterMenu),
      'windowWidth': serializer.toJson<double>(windowWidth),
      'themeMode': serializer.toJson<String>(themeMode),
      'isPaused': serializer.toJson<bool>(isPaused),
      'clearOnExit': serializer.toJson<bool>(clearOnExit),
      'clearSystemClipboard': serializer.toJson<bool>(clearSystemClipboard),
      'ignoreAppsJson': serializer.toJson<String>(ignoreAppsJson),
    };
  }

  AppSetting copyWith({
    int? id,
    bool? launchAtStartup,
    bool? autoCheckUpdates,
    String? hotkeyOpen,
    String? hotkeyPin,
    String? hotkeyDelete,
    bool? autoPaste,
    bool? pastePlain,
    String? searchMode,
    int? historyLimit,
    bool? saveText,
    bool? saveImages,
    bool? saveFiles,
    String? popupPosition,
    String? pinPosition,
    int? imageHeight,
    int? previewDelay,
    String? highlightMatch,
    bool? showSpecialChars,
    bool? showMenuBarIcon,
    String? menuBarIconType,
    bool? showClipboardNearIcon,
    String? showSearchBox,
    bool? showAppName,
    bool? showAppIcon,
    bool? showFooterMenu,
    double? windowWidth,
    String? themeMode,
    bool? isPaused,
    bool? clearOnExit,
    bool? clearSystemClipboard,
    String? ignoreAppsJson,
  }) => AppSetting(
    id: id ?? this.id,
    launchAtStartup: launchAtStartup ?? this.launchAtStartup,
    autoCheckUpdates: autoCheckUpdates ?? this.autoCheckUpdates,
    hotkeyOpen: hotkeyOpen ?? this.hotkeyOpen,
    hotkeyPin: hotkeyPin ?? this.hotkeyPin,
    hotkeyDelete: hotkeyDelete ?? this.hotkeyDelete,
    autoPaste: autoPaste ?? this.autoPaste,
    pastePlain: pastePlain ?? this.pastePlain,
    searchMode: searchMode ?? this.searchMode,
    historyLimit: historyLimit ?? this.historyLimit,
    saveText: saveText ?? this.saveText,
    saveImages: saveImages ?? this.saveImages,
    saveFiles: saveFiles ?? this.saveFiles,
    popupPosition: popupPosition ?? this.popupPosition,
    pinPosition: pinPosition ?? this.pinPosition,
    imageHeight: imageHeight ?? this.imageHeight,
    previewDelay: previewDelay ?? this.previewDelay,
    highlightMatch: highlightMatch ?? this.highlightMatch,
    showSpecialChars: showSpecialChars ?? this.showSpecialChars,
    showMenuBarIcon: showMenuBarIcon ?? this.showMenuBarIcon,
    menuBarIconType: menuBarIconType ?? this.menuBarIconType,
    showClipboardNearIcon: showClipboardNearIcon ?? this.showClipboardNearIcon,
    showSearchBox: showSearchBox ?? this.showSearchBox,
    showAppName: showAppName ?? this.showAppName,
    showAppIcon: showAppIcon ?? this.showAppIcon,
    showFooterMenu: showFooterMenu ?? this.showFooterMenu,
    windowWidth: windowWidth ?? this.windowWidth,
    themeMode: themeMode ?? this.themeMode,
    isPaused: isPaused ?? this.isPaused,
    clearOnExit: clearOnExit ?? this.clearOnExit,
    clearSystemClipboard: clearSystemClipboard ?? this.clearSystemClipboard,
    ignoreAppsJson: ignoreAppsJson ?? this.ignoreAppsJson,
  );
  AppSetting copyWithCompanion(AppSettingsCompanion data) {
    return AppSetting(
      id: data.id.present ? data.id.value : this.id,
      launchAtStartup: data.launchAtStartup.present
          ? data.launchAtStartup.value
          : this.launchAtStartup,
      autoCheckUpdates: data.autoCheckUpdates.present
          ? data.autoCheckUpdates.value
          : this.autoCheckUpdates,
      hotkeyOpen: data.hotkeyOpen.present
          ? data.hotkeyOpen.value
          : this.hotkeyOpen,
      hotkeyPin: data.hotkeyPin.present ? data.hotkeyPin.value : this.hotkeyPin,
      hotkeyDelete: data.hotkeyDelete.present
          ? data.hotkeyDelete.value
          : this.hotkeyDelete,
      autoPaste: data.autoPaste.present ? data.autoPaste.value : this.autoPaste,
      pastePlain: data.pastePlain.present
          ? data.pastePlain.value
          : this.pastePlain,
      searchMode: data.searchMode.present
          ? data.searchMode.value
          : this.searchMode,
      historyLimit: data.historyLimit.present
          ? data.historyLimit.value
          : this.historyLimit,
      saveText: data.saveText.present ? data.saveText.value : this.saveText,
      saveImages: data.saveImages.present
          ? data.saveImages.value
          : this.saveImages,
      saveFiles: data.saveFiles.present ? data.saveFiles.value : this.saveFiles,
      popupPosition: data.popupPosition.present
          ? data.popupPosition.value
          : this.popupPosition,
      pinPosition: data.pinPosition.present
          ? data.pinPosition.value
          : this.pinPosition,
      imageHeight: data.imageHeight.present
          ? data.imageHeight.value
          : this.imageHeight,
      previewDelay: data.previewDelay.present
          ? data.previewDelay.value
          : this.previewDelay,
      highlightMatch: data.highlightMatch.present
          ? data.highlightMatch.value
          : this.highlightMatch,
      showSpecialChars: data.showSpecialChars.present
          ? data.showSpecialChars.value
          : this.showSpecialChars,
      showMenuBarIcon: data.showMenuBarIcon.present
          ? data.showMenuBarIcon.value
          : this.showMenuBarIcon,
      menuBarIconType: data.menuBarIconType.present
          ? data.menuBarIconType.value
          : this.menuBarIconType,
      showClipboardNearIcon: data.showClipboardNearIcon.present
          ? data.showClipboardNearIcon.value
          : this.showClipboardNearIcon,
      showSearchBox: data.showSearchBox.present
          ? data.showSearchBox.value
          : this.showSearchBox,
      showAppName: data.showAppName.present
          ? data.showAppName.value
          : this.showAppName,
      showAppIcon: data.showAppIcon.present
          ? data.showAppIcon.value
          : this.showAppIcon,
      showFooterMenu: data.showFooterMenu.present
          ? data.showFooterMenu.value
          : this.showFooterMenu,
      windowWidth: data.windowWidth.present
          ? data.windowWidth.value
          : this.windowWidth,
      themeMode: data.themeMode.present ? data.themeMode.value : this.themeMode,
      isPaused: data.isPaused.present ? data.isPaused.value : this.isPaused,
      clearOnExit: data.clearOnExit.present
          ? data.clearOnExit.value
          : this.clearOnExit,
      clearSystemClipboard: data.clearSystemClipboard.present
          ? data.clearSystemClipboard.value
          : this.clearSystemClipboard,
      ignoreAppsJson: data.ignoreAppsJson.present
          ? data.ignoreAppsJson.value
          : this.ignoreAppsJson,
    );
  }

  @override
  String toString() {
    return (StringBuffer('AppSetting(')
          ..write('id: $id, ')
          ..write('launchAtStartup: $launchAtStartup, ')
          ..write('autoCheckUpdates: $autoCheckUpdates, ')
          ..write('hotkeyOpen: $hotkeyOpen, ')
          ..write('hotkeyPin: $hotkeyPin, ')
          ..write('hotkeyDelete: $hotkeyDelete, ')
          ..write('autoPaste: $autoPaste, ')
          ..write('pastePlain: $pastePlain, ')
          ..write('searchMode: $searchMode, ')
          ..write('historyLimit: $historyLimit, ')
          ..write('saveText: $saveText, ')
          ..write('saveImages: $saveImages, ')
          ..write('saveFiles: $saveFiles, ')
          ..write('popupPosition: $popupPosition, ')
          ..write('pinPosition: $pinPosition, ')
          ..write('imageHeight: $imageHeight, ')
          ..write('previewDelay: $previewDelay, ')
          ..write('highlightMatch: $highlightMatch, ')
          ..write('showSpecialChars: $showSpecialChars, ')
          ..write('showMenuBarIcon: $showMenuBarIcon, ')
          ..write('menuBarIconType: $menuBarIconType, ')
          ..write('showClipboardNearIcon: $showClipboardNearIcon, ')
          ..write('showSearchBox: $showSearchBox, ')
          ..write('showAppName: $showAppName, ')
          ..write('showAppIcon: $showAppIcon, ')
          ..write('showFooterMenu: $showFooterMenu, ')
          ..write('windowWidth: $windowWidth, ')
          ..write('themeMode: $themeMode, ')
          ..write('isPaused: $isPaused, ')
          ..write('clearOnExit: $clearOnExit, ')
          ..write('clearSystemClipboard: $clearSystemClipboard, ')
          ..write('ignoreAppsJson: $ignoreAppsJson')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hashAll([
    id,
    launchAtStartup,
    autoCheckUpdates,
    hotkeyOpen,
    hotkeyPin,
    hotkeyDelete,
    autoPaste,
    pastePlain,
    searchMode,
    historyLimit,
    saveText,
    saveImages,
    saveFiles,
    popupPosition,
    pinPosition,
    imageHeight,
    previewDelay,
    highlightMatch,
    showSpecialChars,
    showMenuBarIcon,
    menuBarIconType,
    showClipboardNearIcon,
    showSearchBox,
    showAppName,
    showAppIcon,
    showFooterMenu,
    windowWidth,
    themeMode,
    isPaused,
    clearOnExit,
    clearSystemClipboard,
    ignoreAppsJson,
  ]);
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is AppSetting &&
          other.id == this.id &&
          other.launchAtStartup == this.launchAtStartup &&
          other.autoCheckUpdates == this.autoCheckUpdates &&
          other.hotkeyOpen == this.hotkeyOpen &&
          other.hotkeyPin == this.hotkeyPin &&
          other.hotkeyDelete == this.hotkeyDelete &&
          other.autoPaste == this.autoPaste &&
          other.pastePlain == this.pastePlain &&
          other.searchMode == this.searchMode &&
          other.historyLimit == this.historyLimit &&
          other.saveText == this.saveText &&
          other.saveImages == this.saveImages &&
          other.saveFiles == this.saveFiles &&
          other.popupPosition == this.popupPosition &&
          other.pinPosition == this.pinPosition &&
          other.imageHeight == this.imageHeight &&
          other.previewDelay == this.previewDelay &&
          other.highlightMatch == this.highlightMatch &&
          other.showSpecialChars == this.showSpecialChars &&
          other.showMenuBarIcon == this.showMenuBarIcon &&
          other.menuBarIconType == this.menuBarIconType &&
          other.showClipboardNearIcon == this.showClipboardNearIcon &&
          other.showSearchBox == this.showSearchBox &&
          other.showAppName == this.showAppName &&
          other.showAppIcon == this.showAppIcon &&
          other.showFooterMenu == this.showFooterMenu &&
          other.windowWidth == this.windowWidth &&
          other.themeMode == this.themeMode &&
          other.isPaused == this.isPaused &&
          other.clearOnExit == this.clearOnExit &&
          other.clearSystemClipboard == this.clearSystemClipboard &&
          other.ignoreAppsJson == this.ignoreAppsJson);
}

class AppSettingsCompanion extends UpdateCompanion<AppSetting> {
  final Value<int> id;
  final Value<bool> launchAtStartup;
  final Value<bool> autoCheckUpdates;
  final Value<String> hotkeyOpen;
  final Value<String> hotkeyPin;
  final Value<String> hotkeyDelete;
  final Value<bool> autoPaste;
  final Value<bool> pastePlain;
  final Value<String> searchMode;
  final Value<int> historyLimit;
  final Value<bool> saveText;
  final Value<bool> saveImages;
  final Value<bool> saveFiles;
  final Value<String> popupPosition;
  final Value<String> pinPosition;
  final Value<int> imageHeight;
  final Value<int> previewDelay;
  final Value<String> highlightMatch;
  final Value<bool> showSpecialChars;
  final Value<bool> showMenuBarIcon;
  final Value<String> menuBarIconType;
  final Value<bool> showClipboardNearIcon;
  final Value<String> showSearchBox;
  final Value<bool> showAppName;
  final Value<bool> showAppIcon;
  final Value<bool> showFooterMenu;
  final Value<double> windowWidth;
  final Value<String> themeMode;
  final Value<bool> isPaused;
  final Value<bool> clearOnExit;
  final Value<bool> clearSystemClipboard;
  final Value<String> ignoreAppsJson;
  const AppSettingsCompanion({
    this.id = const Value.absent(),
    this.launchAtStartup = const Value.absent(),
    this.autoCheckUpdates = const Value.absent(),
    this.hotkeyOpen = const Value.absent(),
    this.hotkeyPin = const Value.absent(),
    this.hotkeyDelete = const Value.absent(),
    this.autoPaste = const Value.absent(),
    this.pastePlain = const Value.absent(),
    this.searchMode = const Value.absent(),
    this.historyLimit = const Value.absent(),
    this.saveText = const Value.absent(),
    this.saveImages = const Value.absent(),
    this.saveFiles = const Value.absent(),
    this.popupPosition = const Value.absent(),
    this.pinPosition = const Value.absent(),
    this.imageHeight = const Value.absent(),
    this.previewDelay = const Value.absent(),
    this.highlightMatch = const Value.absent(),
    this.showSpecialChars = const Value.absent(),
    this.showMenuBarIcon = const Value.absent(),
    this.menuBarIconType = const Value.absent(),
    this.showClipboardNearIcon = const Value.absent(),
    this.showSearchBox = const Value.absent(),
    this.showAppName = const Value.absent(),
    this.showAppIcon = const Value.absent(),
    this.showFooterMenu = const Value.absent(),
    this.windowWidth = const Value.absent(),
    this.themeMode = const Value.absent(),
    this.isPaused = const Value.absent(),
    this.clearOnExit = const Value.absent(),
    this.clearSystemClipboard = const Value.absent(),
    this.ignoreAppsJson = const Value.absent(),
  });
  AppSettingsCompanion.insert({
    this.id = const Value.absent(),
    this.launchAtStartup = const Value.absent(),
    this.autoCheckUpdates = const Value.absent(),
    this.hotkeyOpen = const Value.absent(),
    this.hotkeyPin = const Value.absent(),
    this.hotkeyDelete = const Value.absent(),
    this.autoPaste = const Value.absent(),
    this.pastePlain = const Value.absent(),
    this.searchMode = const Value.absent(),
    this.historyLimit = const Value.absent(),
    this.saveText = const Value.absent(),
    this.saveImages = const Value.absent(),
    this.saveFiles = const Value.absent(),
    this.popupPosition = const Value.absent(),
    this.pinPosition = const Value.absent(),
    this.imageHeight = const Value.absent(),
    this.previewDelay = const Value.absent(),
    this.highlightMatch = const Value.absent(),
    this.showSpecialChars = const Value.absent(),
    this.showMenuBarIcon = const Value.absent(),
    this.menuBarIconType = const Value.absent(),
    this.showClipboardNearIcon = const Value.absent(),
    this.showSearchBox = const Value.absent(),
    this.showAppName = const Value.absent(),
    this.showAppIcon = const Value.absent(),
    this.showFooterMenu = const Value.absent(),
    this.windowWidth = const Value.absent(),
    this.themeMode = const Value.absent(),
    this.isPaused = const Value.absent(),
    this.clearOnExit = const Value.absent(),
    this.clearSystemClipboard = const Value.absent(),
    this.ignoreAppsJson = const Value.absent(),
  });
  static Insertable<AppSetting> custom({
    Expression<int>? id,
    Expression<bool>? launchAtStartup,
    Expression<bool>? autoCheckUpdates,
    Expression<String>? hotkeyOpen,
    Expression<String>? hotkeyPin,
    Expression<String>? hotkeyDelete,
    Expression<bool>? autoPaste,
    Expression<bool>? pastePlain,
    Expression<String>? searchMode,
    Expression<int>? historyLimit,
    Expression<bool>? saveText,
    Expression<bool>? saveImages,
    Expression<bool>? saveFiles,
    Expression<String>? popupPosition,
    Expression<String>? pinPosition,
    Expression<int>? imageHeight,
    Expression<int>? previewDelay,
    Expression<String>? highlightMatch,
    Expression<bool>? showSpecialChars,
    Expression<bool>? showMenuBarIcon,
    Expression<String>? menuBarIconType,
    Expression<bool>? showClipboardNearIcon,
    Expression<String>? showSearchBox,
    Expression<bool>? showAppName,
    Expression<bool>? showAppIcon,
    Expression<bool>? showFooterMenu,
    Expression<double>? windowWidth,
    Expression<String>? themeMode,
    Expression<bool>? isPaused,
    Expression<bool>? clearOnExit,
    Expression<bool>? clearSystemClipboard,
    Expression<String>? ignoreAppsJson,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (launchAtStartup != null) 'launch_at_startup': launchAtStartup,
      if (autoCheckUpdates != null) 'auto_check_updates': autoCheckUpdates,
      if (hotkeyOpen != null) 'hotkey_open': hotkeyOpen,
      if (hotkeyPin != null) 'hotkey_pin': hotkeyPin,
      if (hotkeyDelete != null) 'hotkey_delete': hotkeyDelete,
      if (autoPaste != null) 'auto_paste': autoPaste,
      if (pastePlain != null) 'paste_plain': pastePlain,
      if (searchMode != null) 'search_mode': searchMode,
      if (historyLimit != null) 'history_limit': historyLimit,
      if (saveText != null) 'save_text': saveText,
      if (saveImages != null) 'save_images': saveImages,
      if (saveFiles != null) 'save_files': saveFiles,
      if (popupPosition != null) 'popup_position': popupPosition,
      if (pinPosition != null) 'pin_position': pinPosition,
      if (imageHeight != null) 'image_height': imageHeight,
      if (previewDelay != null) 'preview_delay': previewDelay,
      if (highlightMatch != null) 'highlight_match': highlightMatch,
      if (showSpecialChars != null) 'show_special_chars': showSpecialChars,
      if (showMenuBarIcon != null) 'show_menu_bar_icon': showMenuBarIcon,
      if (menuBarIconType != null) 'menu_bar_icon_type': menuBarIconType,
      if (showClipboardNearIcon != null)
        'show_clipboard_near_icon': showClipboardNearIcon,
      if (showSearchBox != null) 'show_search_box': showSearchBox,
      if (showAppName != null) 'show_app_name': showAppName,
      if (showAppIcon != null) 'show_app_icon': showAppIcon,
      if (showFooterMenu != null) 'show_footer_menu': showFooterMenu,
      if (windowWidth != null) 'window_width': windowWidth,
      if (themeMode != null) 'theme_mode': themeMode,
      if (isPaused != null) 'is_paused': isPaused,
      if (clearOnExit != null) 'clear_on_exit': clearOnExit,
      if (clearSystemClipboard != null)
        'clear_system_clipboard': clearSystemClipboard,
      if (ignoreAppsJson != null) 'ignore_apps_json': ignoreAppsJson,
    });
  }

  AppSettingsCompanion copyWith({
    Value<int>? id,
    Value<bool>? launchAtStartup,
    Value<bool>? autoCheckUpdates,
    Value<String>? hotkeyOpen,
    Value<String>? hotkeyPin,
    Value<String>? hotkeyDelete,
    Value<bool>? autoPaste,
    Value<bool>? pastePlain,
    Value<String>? searchMode,
    Value<int>? historyLimit,
    Value<bool>? saveText,
    Value<bool>? saveImages,
    Value<bool>? saveFiles,
    Value<String>? popupPosition,
    Value<String>? pinPosition,
    Value<int>? imageHeight,
    Value<int>? previewDelay,
    Value<String>? highlightMatch,
    Value<bool>? showSpecialChars,
    Value<bool>? showMenuBarIcon,
    Value<String>? menuBarIconType,
    Value<bool>? showClipboardNearIcon,
    Value<String>? showSearchBox,
    Value<bool>? showAppName,
    Value<bool>? showAppIcon,
    Value<bool>? showFooterMenu,
    Value<double>? windowWidth,
    Value<String>? themeMode,
    Value<bool>? isPaused,
    Value<bool>? clearOnExit,
    Value<bool>? clearSystemClipboard,
    Value<String>? ignoreAppsJson,
  }) {
    return AppSettingsCompanion(
      id: id ?? this.id,
      launchAtStartup: launchAtStartup ?? this.launchAtStartup,
      autoCheckUpdates: autoCheckUpdates ?? this.autoCheckUpdates,
      hotkeyOpen: hotkeyOpen ?? this.hotkeyOpen,
      hotkeyPin: hotkeyPin ?? this.hotkeyPin,
      hotkeyDelete: hotkeyDelete ?? this.hotkeyDelete,
      autoPaste: autoPaste ?? this.autoPaste,
      pastePlain: pastePlain ?? this.pastePlain,
      searchMode: searchMode ?? this.searchMode,
      historyLimit: historyLimit ?? this.historyLimit,
      saveText: saveText ?? this.saveText,
      saveImages: saveImages ?? this.saveImages,
      saveFiles: saveFiles ?? this.saveFiles,
      popupPosition: popupPosition ?? this.popupPosition,
      pinPosition: pinPosition ?? this.pinPosition,
      imageHeight: imageHeight ?? this.imageHeight,
      previewDelay: previewDelay ?? this.previewDelay,
      highlightMatch: highlightMatch ?? this.highlightMatch,
      showSpecialChars: showSpecialChars ?? this.showSpecialChars,
      showMenuBarIcon: showMenuBarIcon ?? this.showMenuBarIcon,
      menuBarIconType: menuBarIconType ?? this.menuBarIconType,
      showClipboardNearIcon:
          showClipboardNearIcon ?? this.showClipboardNearIcon,
      showSearchBox: showSearchBox ?? this.showSearchBox,
      showAppName: showAppName ?? this.showAppName,
      showAppIcon: showAppIcon ?? this.showAppIcon,
      showFooterMenu: showFooterMenu ?? this.showFooterMenu,
      windowWidth: windowWidth ?? this.windowWidth,
      themeMode: themeMode ?? this.themeMode,
      isPaused: isPaused ?? this.isPaused,
      clearOnExit: clearOnExit ?? this.clearOnExit,
      clearSystemClipboard: clearSystemClipboard ?? this.clearSystemClipboard,
      ignoreAppsJson: ignoreAppsJson ?? this.ignoreAppsJson,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (launchAtStartup.present) {
      map['launch_at_startup'] = Variable<bool>(launchAtStartup.value);
    }
    if (autoCheckUpdates.present) {
      map['auto_check_updates'] = Variable<bool>(autoCheckUpdates.value);
    }
    if (hotkeyOpen.present) {
      map['hotkey_open'] = Variable<String>(hotkeyOpen.value);
    }
    if (hotkeyPin.present) {
      map['hotkey_pin'] = Variable<String>(hotkeyPin.value);
    }
    if (hotkeyDelete.present) {
      map['hotkey_delete'] = Variable<String>(hotkeyDelete.value);
    }
    if (autoPaste.present) {
      map['auto_paste'] = Variable<bool>(autoPaste.value);
    }
    if (pastePlain.present) {
      map['paste_plain'] = Variable<bool>(pastePlain.value);
    }
    if (searchMode.present) {
      map['search_mode'] = Variable<String>(searchMode.value);
    }
    if (historyLimit.present) {
      map['history_limit'] = Variable<int>(historyLimit.value);
    }
    if (saveText.present) {
      map['save_text'] = Variable<bool>(saveText.value);
    }
    if (saveImages.present) {
      map['save_images'] = Variable<bool>(saveImages.value);
    }
    if (saveFiles.present) {
      map['save_files'] = Variable<bool>(saveFiles.value);
    }
    if (popupPosition.present) {
      map['popup_position'] = Variable<String>(popupPosition.value);
    }
    if (pinPosition.present) {
      map['pin_position'] = Variable<String>(pinPosition.value);
    }
    if (imageHeight.present) {
      map['image_height'] = Variable<int>(imageHeight.value);
    }
    if (previewDelay.present) {
      map['preview_delay'] = Variable<int>(previewDelay.value);
    }
    if (highlightMatch.present) {
      map['highlight_match'] = Variable<String>(highlightMatch.value);
    }
    if (showSpecialChars.present) {
      map['show_special_chars'] = Variable<bool>(showSpecialChars.value);
    }
    if (showMenuBarIcon.present) {
      map['show_menu_bar_icon'] = Variable<bool>(showMenuBarIcon.value);
    }
    if (menuBarIconType.present) {
      map['menu_bar_icon_type'] = Variable<String>(menuBarIconType.value);
    }
    if (showClipboardNearIcon.present) {
      map['show_clipboard_near_icon'] = Variable<bool>(
        showClipboardNearIcon.value,
      );
    }
    if (showSearchBox.present) {
      map['show_search_box'] = Variable<String>(showSearchBox.value);
    }
    if (showAppName.present) {
      map['show_app_name'] = Variable<bool>(showAppName.value);
    }
    if (showAppIcon.present) {
      map['show_app_icon'] = Variable<bool>(showAppIcon.value);
    }
    if (showFooterMenu.present) {
      map['show_footer_menu'] = Variable<bool>(showFooterMenu.value);
    }
    if (windowWidth.present) {
      map['window_width'] = Variable<double>(windowWidth.value);
    }
    if (themeMode.present) {
      map['theme_mode'] = Variable<String>(themeMode.value);
    }
    if (isPaused.present) {
      map['is_paused'] = Variable<bool>(isPaused.value);
    }
    if (clearOnExit.present) {
      map['clear_on_exit'] = Variable<bool>(clearOnExit.value);
    }
    if (clearSystemClipboard.present) {
      map['clear_system_clipboard'] = Variable<bool>(
        clearSystemClipboard.value,
      );
    }
    if (ignoreAppsJson.present) {
      map['ignore_apps_json'] = Variable<String>(ignoreAppsJson.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('AppSettingsCompanion(')
          ..write('id: $id, ')
          ..write('launchAtStartup: $launchAtStartup, ')
          ..write('autoCheckUpdates: $autoCheckUpdates, ')
          ..write('hotkeyOpen: $hotkeyOpen, ')
          ..write('hotkeyPin: $hotkeyPin, ')
          ..write('hotkeyDelete: $hotkeyDelete, ')
          ..write('autoPaste: $autoPaste, ')
          ..write('pastePlain: $pastePlain, ')
          ..write('searchMode: $searchMode, ')
          ..write('historyLimit: $historyLimit, ')
          ..write('saveText: $saveText, ')
          ..write('saveImages: $saveImages, ')
          ..write('saveFiles: $saveFiles, ')
          ..write('popupPosition: $popupPosition, ')
          ..write('pinPosition: $pinPosition, ')
          ..write('imageHeight: $imageHeight, ')
          ..write('previewDelay: $previewDelay, ')
          ..write('highlightMatch: $highlightMatch, ')
          ..write('showSpecialChars: $showSpecialChars, ')
          ..write('showMenuBarIcon: $showMenuBarIcon, ')
          ..write('menuBarIconType: $menuBarIconType, ')
          ..write('showClipboardNearIcon: $showClipboardNearIcon, ')
          ..write('showSearchBox: $showSearchBox, ')
          ..write('showAppName: $showAppName, ')
          ..write('showAppIcon: $showAppIcon, ')
          ..write('showFooterMenu: $showFooterMenu, ')
          ..write('windowWidth: $windowWidth, ')
          ..write('themeMode: $themeMode, ')
          ..write('isPaused: $isPaused, ')
          ..write('clearOnExit: $clearOnExit, ')
          ..write('clearSystemClipboard: $clearSystemClipboard, ')
          ..write('ignoreAppsJson: $ignoreAppsJson')
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
      Value<bool> launchAtStartup,
      Value<bool> autoCheckUpdates,
      Value<String> hotkeyOpen,
      Value<String> hotkeyPin,
      Value<String> hotkeyDelete,
      Value<bool> autoPaste,
      Value<bool> pastePlain,
      Value<String> searchMode,
      Value<int> historyLimit,
      Value<bool> saveText,
      Value<bool> saveImages,
      Value<bool> saveFiles,
      Value<String> popupPosition,
      Value<String> pinPosition,
      Value<int> imageHeight,
      Value<int> previewDelay,
      Value<String> highlightMatch,
      Value<bool> showSpecialChars,
      Value<bool> showMenuBarIcon,
      Value<String> menuBarIconType,
      Value<bool> showClipboardNearIcon,
      Value<String> showSearchBox,
      Value<bool> showAppName,
      Value<bool> showAppIcon,
      Value<bool> showFooterMenu,
      Value<double> windowWidth,
      Value<String> themeMode,
      Value<bool> isPaused,
      Value<bool> clearOnExit,
      Value<bool> clearSystemClipboard,
      Value<String> ignoreAppsJson,
    });
typedef $$AppSettingsTableUpdateCompanionBuilder =
    AppSettingsCompanion Function({
      Value<int> id,
      Value<bool> launchAtStartup,
      Value<bool> autoCheckUpdates,
      Value<String> hotkeyOpen,
      Value<String> hotkeyPin,
      Value<String> hotkeyDelete,
      Value<bool> autoPaste,
      Value<bool> pastePlain,
      Value<String> searchMode,
      Value<int> historyLimit,
      Value<bool> saveText,
      Value<bool> saveImages,
      Value<bool> saveFiles,
      Value<String> popupPosition,
      Value<String> pinPosition,
      Value<int> imageHeight,
      Value<int> previewDelay,
      Value<String> highlightMatch,
      Value<bool> showSpecialChars,
      Value<bool> showMenuBarIcon,
      Value<String> menuBarIconType,
      Value<bool> showClipboardNearIcon,
      Value<String> showSearchBox,
      Value<bool> showAppName,
      Value<bool> showAppIcon,
      Value<bool> showFooterMenu,
      Value<double> windowWidth,
      Value<String> themeMode,
      Value<bool> isPaused,
      Value<bool> clearOnExit,
      Value<bool> clearSystemClipboard,
      Value<String> ignoreAppsJson,
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

  ColumnFilters<bool> get launchAtStartup => $composableBuilder(
    column: $table.launchAtStartup,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get autoCheckUpdates => $composableBuilder(
    column: $table.autoCheckUpdates,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get hotkeyOpen => $composableBuilder(
    column: $table.hotkeyOpen,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get hotkeyPin => $composableBuilder(
    column: $table.hotkeyPin,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get hotkeyDelete => $composableBuilder(
    column: $table.hotkeyDelete,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get autoPaste => $composableBuilder(
    column: $table.autoPaste,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get pastePlain => $composableBuilder(
    column: $table.pastePlain,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get searchMode => $composableBuilder(
    column: $table.searchMode,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get historyLimit => $composableBuilder(
    column: $table.historyLimit,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get saveText => $composableBuilder(
    column: $table.saveText,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get saveImages => $composableBuilder(
    column: $table.saveImages,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get saveFiles => $composableBuilder(
    column: $table.saveFiles,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get popupPosition => $composableBuilder(
    column: $table.popupPosition,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get pinPosition => $composableBuilder(
    column: $table.pinPosition,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get imageHeight => $composableBuilder(
    column: $table.imageHeight,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get previewDelay => $composableBuilder(
    column: $table.previewDelay,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get highlightMatch => $composableBuilder(
    column: $table.highlightMatch,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get showSpecialChars => $composableBuilder(
    column: $table.showSpecialChars,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get showMenuBarIcon => $composableBuilder(
    column: $table.showMenuBarIcon,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get menuBarIconType => $composableBuilder(
    column: $table.menuBarIconType,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get showClipboardNearIcon => $composableBuilder(
    column: $table.showClipboardNearIcon,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get showSearchBox => $composableBuilder(
    column: $table.showSearchBox,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get showAppName => $composableBuilder(
    column: $table.showAppName,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get showAppIcon => $composableBuilder(
    column: $table.showAppIcon,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get showFooterMenu => $composableBuilder(
    column: $table.showFooterMenu,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<double> get windowWidth => $composableBuilder(
    column: $table.windowWidth,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get themeMode => $composableBuilder(
    column: $table.themeMode,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get isPaused => $composableBuilder(
    column: $table.isPaused,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get clearOnExit => $composableBuilder(
    column: $table.clearOnExit,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<bool> get clearSystemClipboard => $composableBuilder(
    column: $table.clearSystemClipboard,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get ignoreAppsJson => $composableBuilder(
    column: $table.ignoreAppsJson,
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

  ColumnOrderings<bool> get launchAtStartup => $composableBuilder(
    column: $table.launchAtStartup,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get autoCheckUpdates => $composableBuilder(
    column: $table.autoCheckUpdates,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get hotkeyOpen => $composableBuilder(
    column: $table.hotkeyOpen,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get hotkeyPin => $composableBuilder(
    column: $table.hotkeyPin,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get hotkeyDelete => $composableBuilder(
    column: $table.hotkeyDelete,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get autoPaste => $composableBuilder(
    column: $table.autoPaste,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get pastePlain => $composableBuilder(
    column: $table.pastePlain,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get searchMode => $composableBuilder(
    column: $table.searchMode,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get historyLimit => $composableBuilder(
    column: $table.historyLimit,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get saveText => $composableBuilder(
    column: $table.saveText,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get saveImages => $composableBuilder(
    column: $table.saveImages,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get saveFiles => $composableBuilder(
    column: $table.saveFiles,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get popupPosition => $composableBuilder(
    column: $table.popupPosition,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get pinPosition => $composableBuilder(
    column: $table.pinPosition,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get imageHeight => $composableBuilder(
    column: $table.imageHeight,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get previewDelay => $composableBuilder(
    column: $table.previewDelay,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get highlightMatch => $composableBuilder(
    column: $table.highlightMatch,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get showSpecialChars => $composableBuilder(
    column: $table.showSpecialChars,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get showMenuBarIcon => $composableBuilder(
    column: $table.showMenuBarIcon,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get menuBarIconType => $composableBuilder(
    column: $table.menuBarIconType,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get showClipboardNearIcon => $composableBuilder(
    column: $table.showClipboardNearIcon,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get showSearchBox => $composableBuilder(
    column: $table.showSearchBox,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get showAppName => $composableBuilder(
    column: $table.showAppName,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get showAppIcon => $composableBuilder(
    column: $table.showAppIcon,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get showFooterMenu => $composableBuilder(
    column: $table.showFooterMenu,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<double> get windowWidth => $composableBuilder(
    column: $table.windowWidth,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get themeMode => $composableBuilder(
    column: $table.themeMode,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get isPaused => $composableBuilder(
    column: $table.isPaused,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get clearOnExit => $composableBuilder(
    column: $table.clearOnExit,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<bool> get clearSystemClipboard => $composableBuilder(
    column: $table.clearSystemClipboard,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get ignoreAppsJson => $composableBuilder(
    column: $table.ignoreAppsJson,
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

  GeneratedColumn<bool> get launchAtStartup => $composableBuilder(
    column: $table.launchAtStartup,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get autoCheckUpdates => $composableBuilder(
    column: $table.autoCheckUpdates,
    builder: (column) => column,
  );

  GeneratedColumn<String> get hotkeyOpen => $composableBuilder(
    column: $table.hotkeyOpen,
    builder: (column) => column,
  );

  GeneratedColumn<String> get hotkeyPin =>
      $composableBuilder(column: $table.hotkeyPin, builder: (column) => column);

  GeneratedColumn<String> get hotkeyDelete => $composableBuilder(
    column: $table.hotkeyDelete,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get autoPaste =>
      $composableBuilder(column: $table.autoPaste, builder: (column) => column);

  GeneratedColumn<bool> get pastePlain => $composableBuilder(
    column: $table.pastePlain,
    builder: (column) => column,
  );

  GeneratedColumn<String> get searchMode => $composableBuilder(
    column: $table.searchMode,
    builder: (column) => column,
  );

  GeneratedColumn<int> get historyLimit => $composableBuilder(
    column: $table.historyLimit,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get saveText =>
      $composableBuilder(column: $table.saveText, builder: (column) => column);

  GeneratedColumn<bool> get saveImages => $composableBuilder(
    column: $table.saveImages,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get saveFiles =>
      $composableBuilder(column: $table.saveFiles, builder: (column) => column);

  GeneratedColumn<String> get popupPosition => $composableBuilder(
    column: $table.popupPosition,
    builder: (column) => column,
  );

  GeneratedColumn<String> get pinPosition => $composableBuilder(
    column: $table.pinPosition,
    builder: (column) => column,
  );

  GeneratedColumn<int> get imageHeight => $composableBuilder(
    column: $table.imageHeight,
    builder: (column) => column,
  );

  GeneratedColumn<int> get previewDelay => $composableBuilder(
    column: $table.previewDelay,
    builder: (column) => column,
  );

  GeneratedColumn<String> get highlightMatch => $composableBuilder(
    column: $table.highlightMatch,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get showSpecialChars => $composableBuilder(
    column: $table.showSpecialChars,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get showMenuBarIcon => $composableBuilder(
    column: $table.showMenuBarIcon,
    builder: (column) => column,
  );

  GeneratedColumn<String> get menuBarIconType => $composableBuilder(
    column: $table.menuBarIconType,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get showClipboardNearIcon => $composableBuilder(
    column: $table.showClipboardNearIcon,
    builder: (column) => column,
  );

  GeneratedColumn<String> get showSearchBox => $composableBuilder(
    column: $table.showSearchBox,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get showAppName => $composableBuilder(
    column: $table.showAppName,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get showAppIcon => $composableBuilder(
    column: $table.showAppIcon,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get showFooterMenu => $composableBuilder(
    column: $table.showFooterMenu,
    builder: (column) => column,
  );

  GeneratedColumn<double> get windowWidth => $composableBuilder(
    column: $table.windowWidth,
    builder: (column) => column,
  );

  GeneratedColumn<String> get themeMode =>
      $composableBuilder(column: $table.themeMode, builder: (column) => column);

  GeneratedColumn<bool> get isPaused =>
      $composableBuilder(column: $table.isPaused, builder: (column) => column);

  GeneratedColumn<bool> get clearOnExit => $composableBuilder(
    column: $table.clearOnExit,
    builder: (column) => column,
  );

  GeneratedColumn<bool> get clearSystemClipboard => $composableBuilder(
    column: $table.clearSystemClipboard,
    builder: (column) => column,
  );

  GeneratedColumn<String> get ignoreAppsJson => $composableBuilder(
    column: $table.ignoreAppsJson,
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
                Value<bool> launchAtStartup = const Value.absent(),
                Value<bool> autoCheckUpdates = const Value.absent(),
                Value<String> hotkeyOpen = const Value.absent(),
                Value<String> hotkeyPin = const Value.absent(),
                Value<String> hotkeyDelete = const Value.absent(),
                Value<bool> autoPaste = const Value.absent(),
                Value<bool> pastePlain = const Value.absent(),
                Value<String> searchMode = const Value.absent(),
                Value<int> historyLimit = const Value.absent(),
                Value<bool> saveText = const Value.absent(),
                Value<bool> saveImages = const Value.absent(),
                Value<bool> saveFiles = const Value.absent(),
                Value<String> popupPosition = const Value.absent(),
                Value<String> pinPosition = const Value.absent(),
                Value<int> imageHeight = const Value.absent(),
                Value<int> previewDelay = const Value.absent(),
                Value<String> highlightMatch = const Value.absent(),
                Value<bool> showSpecialChars = const Value.absent(),
                Value<bool> showMenuBarIcon = const Value.absent(),
                Value<String> menuBarIconType = const Value.absent(),
                Value<bool> showClipboardNearIcon = const Value.absent(),
                Value<String> showSearchBox = const Value.absent(),
                Value<bool> showAppName = const Value.absent(),
                Value<bool> showAppIcon = const Value.absent(),
                Value<bool> showFooterMenu = const Value.absent(),
                Value<double> windowWidth = const Value.absent(),
                Value<String> themeMode = const Value.absent(),
                Value<bool> isPaused = const Value.absent(),
                Value<bool> clearOnExit = const Value.absent(),
                Value<bool> clearSystemClipboard = const Value.absent(),
                Value<String> ignoreAppsJson = const Value.absent(),
              }) => AppSettingsCompanion(
                id: id,
                launchAtStartup: launchAtStartup,
                autoCheckUpdates: autoCheckUpdates,
                hotkeyOpen: hotkeyOpen,
                hotkeyPin: hotkeyPin,
                hotkeyDelete: hotkeyDelete,
                autoPaste: autoPaste,
                pastePlain: pastePlain,
                searchMode: searchMode,
                historyLimit: historyLimit,
                saveText: saveText,
                saveImages: saveImages,
                saveFiles: saveFiles,
                popupPosition: popupPosition,
                pinPosition: pinPosition,
                imageHeight: imageHeight,
                previewDelay: previewDelay,
                highlightMatch: highlightMatch,
                showSpecialChars: showSpecialChars,
                showMenuBarIcon: showMenuBarIcon,
                menuBarIconType: menuBarIconType,
                showClipboardNearIcon: showClipboardNearIcon,
                showSearchBox: showSearchBox,
                showAppName: showAppName,
                showAppIcon: showAppIcon,
                showFooterMenu: showFooterMenu,
                windowWidth: windowWidth,
                themeMode: themeMode,
                isPaused: isPaused,
                clearOnExit: clearOnExit,
                clearSystemClipboard: clearSystemClipboard,
                ignoreAppsJson: ignoreAppsJson,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<bool> launchAtStartup = const Value.absent(),
                Value<bool> autoCheckUpdates = const Value.absent(),
                Value<String> hotkeyOpen = const Value.absent(),
                Value<String> hotkeyPin = const Value.absent(),
                Value<String> hotkeyDelete = const Value.absent(),
                Value<bool> autoPaste = const Value.absent(),
                Value<bool> pastePlain = const Value.absent(),
                Value<String> searchMode = const Value.absent(),
                Value<int> historyLimit = const Value.absent(),
                Value<bool> saveText = const Value.absent(),
                Value<bool> saveImages = const Value.absent(),
                Value<bool> saveFiles = const Value.absent(),
                Value<String> popupPosition = const Value.absent(),
                Value<String> pinPosition = const Value.absent(),
                Value<int> imageHeight = const Value.absent(),
                Value<int> previewDelay = const Value.absent(),
                Value<String> highlightMatch = const Value.absent(),
                Value<bool> showSpecialChars = const Value.absent(),
                Value<bool> showMenuBarIcon = const Value.absent(),
                Value<String> menuBarIconType = const Value.absent(),
                Value<bool> showClipboardNearIcon = const Value.absent(),
                Value<String> showSearchBox = const Value.absent(),
                Value<bool> showAppName = const Value.absent(),
                Value<bool> showAppIcon = const Value.absent(),
                Value<bool> showFooterMenu = const Value.absent(),
                Value<double> windowWidth = const Value.absent(),
                Value<String> themeMode = const Value.absent(),
                Value<bool> isPaused = const Value.absent(),
                Value<bool> clearOnExit = const Value.absent(),
                Value<bool> clearSystemClipboard = const Value.absent(),
                Value<String> ignoreAppsJson = const Value.absent(),
              }) => AppSettingsCompanion.insert(
                id: id,
                launchAtStartup: launchAtStartup,
                autoCheckUpdates: autoCheckUpdates,
                hotkeyOpen: hotkeyOpen,
                hotkeyPin: hotkeyPin,
                hotkeyDelete: hotkeyDelete,
                autoPaste: autoPaste,
                pastePlain: pastePlain,
                searchMode: searchMode,
                historyLimit: historyLimit,
                saveText: saveText,
                saveImages: saveImages,
                saveFiles: saveFiles,
                popupPosition: popupPosition,
                pinPosition: pinPosition,
                imageHeight: imageHeight,
                previewDelay: previewDelay,
                highlightMatch: highlightMatch,
                showSpecialChars: showSpecialChars,
                showMenuBarIcon: showMenuBarIcon,
                menuBarIconType: menuBarIconType,
                showClipboardNearIcon: showClipboardNearIcon,
                showSearchBox: showSearchBox,
                showAppName: showAppName,
                showAppIcon: showAppIcon,
                showFooterMenu: showFooterMenu,
                windowWidth: windowWidth,
                themeMode: themeMode,
                isPaused: isPaused,
                clearOnExit: clearOnExit,
                clearSystemClipboard: clearSystemClipboard,
                ignoreAppsJson: ignoreAppsJson,
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
