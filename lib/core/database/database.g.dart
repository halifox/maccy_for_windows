// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'database.dart';

// ignore_for_file: type=lint
class $HistoryItemsTable extends HistoryItems
    with TableInfo<$HistoryItemsTable, HistoryItem> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $HistoryItemsTable(this.attachedDatabase, [this._alias]);
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
  static const VerificationMeta _applicationMeta = const VerificationMeta(
    'application',
  );
  @override
  late final GeneratedColumn<String> application = GeneratedColumn<String>(
    'application',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _firstCopiedAtMeta = const VerificationMeta(
    'firstCopiedAt',
  );
  @override
  late final GeneratedColumn<DateTime> firstCopiedAt =
      GeneratedColumn<DateTime>(
        'first_copied_at',
        aliasedName,
        false,
        type: DriftSqlType.dateTime,
        requiredDuringInsert: true,
      );
  static const VerificationMeta _lastCopiedAtMeta = const VerificationMeta(
    'lastCopiedAt',
  );
  @override
  late final GeneratedColumn<DateTime> lastCopiedAt = GeneratedColumn<DateTime>(
    'last_copied_at',
    aliasedName,
    false,
    type: DriftSqlType.dateTime,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _numberOfCopiesMeta = const VerificationMeta(
    'numberOfCopies',
  );
  @override
  late final GeneratedColumn<int> numberOfCopies = GeneratedColumn<int>(
    'number_of_copies',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: false,
    defaultValue: const Constant(1),
  );
  static const VerificationMeta _pinMeta = const VerificationMeta('pin');
  @override
  late final GeneratedColumn<String> pin = GeneratedColumn<String>(
    'pin',
    aliasedName,
    true,
    additionalChecks: GeneratedColumn.checkTextLength(
      minTextLength: 1,
      maxTextLength: 1,
    ),
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  static const VerificationMeta _titleMeta = const VerificationMeta('title');
  @override
  late final GeneratedColumn<String> title = GeneratedColumn<String>(
    'title',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _aliasMeta = const VerificationMeta('alias');
  @override
  late final GeneratedColumn<String> alias = GeneratedColumn<String>(
    'alias',
    aliasedName,
    true,
    type: DriftSqlType.string,
    requiredDuringInsert: false,
  );
  @override
  List<GeneratedColumn> get $columns => [
    id,
    application,
    firstCopiedAt,
    lastCopiedAt,
    numberOfCopies,
    pin,
    title,
    alias,
  ];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'history_items';
  @override
  VerificationContext validateIntegrity(
    Insertable<HistoryItem> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('application')) {
      context.handle(
        _applicationMeta,
        application.isAcceptableOrUnknown(
          data['application']!,
          _applicationMeta,
        ),
      );
    }
    if (data.containsKey('first_copied_at')) {
      context.handle(
        _firstCopiedAtMeta,
        firstCopiedAt.isAcceptableOrUnknown(
          data['first_copied_at']!,
          _firstCopiedAtMeta,
        ),
      );
    } else if (isInserting) {
      context.missing(_firstCopiedAtMeta);
    }
    if (data.containsKey('last_copied_at')) {
      context.handle(
        _lastCopiedAtMeta,
        lastCopiedAt.isAcceptableOrUnknown(
          data['last_copied_at']!,
          _lastCopiedAtMeta,
        ),
      );
    } else if (isInserting) {
      context.missing(_lastCopiedAtMeta);
    }
    if (data.containsKey('number_of_copies')) {
      context.handle(
        _numberOfCopiesMeta,
        numberOfCopies.isAcceptableOrUnknown(
          data['number_of_copies']!,
          _numberOfCopiesMeta,
        ),
      );
    }
    if (data.containsKey('pin')) {
      context.handle(
        _pinMeta,
        pin.isAcceptableOrUnknown(data['pin']!, _pinMeta),
      );
    }
    if (data.containsKey('title')) {
      context.handle(
        _titleMeta,
        title.isAcceptableOrUnknown(data['title']!, _titleMeta),
      );
    } else if (isInserting) {
      context.missing(_titleMeta);
    }
    if (data.containsKey('alias')) {
      context.handle(
        _aliasMeta,
        alias.isAcceptableOrUnknown(data['alias']!, _aliasMeta),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  HistoryItem map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return HistoryItem(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      application: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}application'],
      ),
      firstCopiedAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}first_copied_at'],
      )!,
      lastCopiedAt: attachedDatabase.typeMapping.read(
        DriftSqlType.dateTime,
        data['${effectivePrefix}last_copied_at'],
      )!,
      numberOfCopies: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}number_of_copies'],
      )!,
      pin: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}pin'],
      ),
      title: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}title'],
      )!,
      alias: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}alias'],
      ),
    );
  }

  @override
  $HistoryItemsTable createAlias(String alias) {
    return $HistoryItemsTable(attachedDatabase, alias);
  }
}

class HistoryItem extends DataClass implements Insertable<HistoryItem> {
  final int id;
  final String? application;
  final DateTime firstCopiedAt;
  final DateTime lastCopiedAt;
  final int numberOfCopies;
  final String? pin;
  final String title;
  final String? alias;
  const HistoryItem({
    required this.id,
    this.application,
    required this.firstCopiedAt,
    required this.lastCopiedAt,
    required this.numberOfCopies,
    this.pin,
    required this.title,
    this.alias,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    if (!nullToAbsent || application != null) {
      map['application'] = Variable<String>(application);
    }
    map['first_copied_at'] = Variable<DateTime>(firstCopiedAt);
    map['last_copied_at'] = Variable<DateTime>(lastCopiedAt);
    map['number_of_copies'] = Variable<int>(numberOfCopies);
    if (!nullToAbsent || pin != null) {
      map['pin'] = Variable<String>(pin);
    }
    map['title'] = Variable<String>(title);
    if (!nullToAbsent || alias != null) {
      map['alias'] = Variable<String>(alias);
    }
    return map;
  }

  HistoryItemsCompanion toCompanion(bool nullToAbsent) {
    return HistoryItemsCompanion(
      id: Value(id),
      application: application == null && nullToAbsent
          ? const Value.absent()
          : Value(application),
      firstCopiedAt: Value(firstCopiedAt),
      lastCopiedAt: Value(lastCopiedAt),
      numberOfCopies: Value(numberOfCopies),
      pin: pin == null && nullToAbsent ? const Value.absent() : Value(pin),
      title: Value(title),
      alias: alias == null && nullToAbsent
          ? const Value.absent()
          : Value(alias),
    );
  }

  factory HistoryItem.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return HistoryItem(
      id: serializer.fromJson<int>(json['id']),
      application: serializer.fromJson<String?>(json['application']),
      firstCopiedAt: serializer.fromJson<DateTime>(json['firstCopiedAt']),
      lastCopiedAt: serializer.fromJson<DateTime>(json['lastCopiedAt']),
      numberOfCopies: serializer.fromJson<int>(json['numberOfCopies']),
      pin: serializer.fromJson<String?>(json['pin']),
      title: serializer.fromJson<String>(json['title']),
      alias: serializer.fromJson<String?>(json['alias']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'application': serializer.toJson<String?>(application),
      'firstCopiedAt': serializer.toJson<DateTime>(firstCopiedAt),
      'lastCopiedAt': serializer.toJson<DateTime>(lastCopiedAt),
      'numberOfCopies': serializer.toJson<int>(numberOfCopies),
      'pin': serializer.toJson<String?>(pin),
      'title': serializer.toJson<String>(title),
      'alias': serializer.toJson<String?>(alias),
    };
  }

  HistoryItem copyWith({
    int? id,
    Value<String?> application = const Value.absent(),
    DateTime? firstCopiedAt,
    DateTime? lastCopiedAt,
    int? numberOfCopies,
    Value<String?> pin = const Value.absent(),
    String? title,
    Value<String?> alias = const Value.absent(),
  }) => HistoryItem(
    id: id ?? this.id,
    application: application.present ? application.value : this.application,
    firstCopiedAt: firstCopiedAt ?? this.firstCopiedAt,
    lastCopiedAt: lastCopiedAt ?? this.lastCopiedAt,
    numberOfCopies: numberOfCopies ?? this.numberOfCopies,
    pin: pin.present ? pin.value : this.pin,
    title: title ?? this.title,
    alias: alias.present ? alias.value : this.alias,
  );
  HistoryItem copyWithCompanion(HistoryItemsCompanion data) {
    return HistoryItem(
      id: data.id.present ? data.id.value : this.id,
      application: data.application.present
          ? data.application.value
          : this.application,
      firstCopiedAt: data.firstCopiedAt.present
          ? data.firstCopiedAt.value
          : this.firstCopiedAt,
      lastCopiedAt: data.lastCopiedAt.present
          ? data.lastCopiedAt.value
          : this.lastCopiedAt,
      numberOfCopies: data.numberOfCopies.present
          ? data.numberOfCopies.value
          : this.numberOfCopies,
      pin: data.pin.present ? data.pin.value : this.pin,
      title: data.title.present ? data.title.value : this.title,
      alias: data.alias.present ? data.alias.value : this.alias,
    );
  }

  @override
  String toString() {
    return (StringBuffer('HistoryItem(')
          ..write('id: $id, ')
          ..write('application: $application, ')
          ..write('firstCopiedAt: $firstCopiedAt, ')
          ..write('lastCopiedAt: $lastCopiedAt, ')
          ..write('numberOfCopies: $numberOfCopies, ')
          ..write('pin: $pin, ')
          ..write('title: $title, ')
          ..write('alias: $alias')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode => Object.hash(
    id,
    application,
    firstCopiedAt,
    lastCopiedAt,
    numberOfCopies,
    pin,
    title,
    alias,
  );
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is HistoryItem &&
          other.id == this.id &&
          other.application == this.application &&
          other.firstCopiedAt == this.firstCopiedAt &&
          other.lastCopiedAt == this.lastCopiedAt &&
          other.numberOfCopies == this.numberOfCopies &&
          other.pin == this.pin &&
          other.title == this.title &&
          other.alias == this.alias);
}

class HistoryItemsCompanion extends UpdateCompanion<HistoryItem> {
  final Value<int> id;
  final Value<String?> application;
  final Value<DateTime> firstCopiedAt;
  final Value<DateTime> lastCopiedAt;
  final Value<int> numberOfCopies;
  final Value<String?> pin;
  final Value<String> title;
  final Value<String?> alias;
  const HistoryItemsCompanion({
    this.id = const Value.absent(),
    this.application = const Value.absent(),
    this.firstCopiedAt = const Value.absent(),
    this.lastCopiedAt = const Value.absent(),
    this.numberOfCopies = const Value.absent(),
    this.pin = const Value.absent(),
    this.title = const Value.absent(),
    this.alias = const Value.absent(),
  });
  HistoryItemsCompanion.insert({
    this.id = const Value.absent(),
    this.application = const Value.absent(),
    required DateTime firstCopiedAt,
    required DateTime lastCopiedAt,
    this.numberOfCopies = const Value.absent(),
    this.pin = const Value.absent(),
    required String title,
    this.alias = const Value.absent(),
  }) : firstCopiedAt = Value(firstCopiedAt),
       lastCopiedAt = Value(lastCopiedAt),
       title = Value(title);
  static Insertable<HistoryItem> custom({
    Expression<int>? id,
    Expression<String>? application,
    Expression<DateTime>? firstCopiedAt,
    Expression<DateTime>? lastCopiedAt,
    Expression<int>? numberOfCopies,
    Expression<String>? pin,
    Expression<String>? title,
    Expression<String>? alias,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (application != null) 'application': application,
      if (firstCopiedAt != null) 'first_copied_at': firstCopiedAt,
      if (lastCopiedAt != null) 'last_copied_at': lastCopiedAt,
      if (numberOfCopies != null) 'number_of_copies': numberOfCopies,
      if (pin != null) 'pin': pin,
      if (title != null) 'title': title,
      if (alias != null) 'alias': alias,
    });
  }

  HistoryItemsCompanion copyWith({
    Value<int>? id,
    Value<String?>? application,
    Value<DateTime>? firstCopiedAt,
    Value<DateTime>? lastCopiedAt,
    Value<int>? numberOfCopies,
    Value<String?>? pin,
    Value<String>? title,
    Value<String?>? alias,
  }) {
    return HistoryItemsCompanion(
      id: id ?? this.id,
      application: application ?? this.application,
      firstCopiedAt: firstCopiedAt ?? this.firstCopiedAt,
      lastCopiedAt: lastCopiedAt ?? this.lastCopiedAt,
      numberOfCopies: numberOfCopies ?? this.numberOfCopies,
      pin: pin ?? this.pin,
      title: title ?? this.title,
      alias: alias ?? this.alias,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (application.present) {
      map['application'] = Variable<String>(application.value);
    }
    if (firstCopiedAt.present) {
      map['first_copied_at'] = Variable<DateTime>(firstCopiedAt.value);
    }
    if (lastCopiedAt.present) {
      map['last_copied_at'] = Variable<DateTime>(lastCopiedAt.value);
    }
    if (numberOfCopies.present) {
      map['number_of_copies'] = Variable<int>(numberOfCopies.value);
    }
    if (pin.present) {
      map['pin'] = Variable<String>(pin.value);
    }
    if (title.present) {
      map['title'] = Variable<String>(title.value);
    }
    if (alias.present) {
      map['alias'] = Variable<String>(alias.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('HistoryItemsCompanion(')
          ..write('id: $id, ')
          ..write('application: $application, ')
          ..write('firstCopiedAt: $firstCopiedAt, ')
          ..write('lastCopiedAt: $lastCopiedAt, ')
          ..write('numberOfCopies: $numberOfCopies, ')
          ..write('pin: $pin, ')
          ..write('title: $title, ')
          ..write('alias: $alias')
          ..write(')'))
        .toString();
  }
}

class $HistoryItemContentsTable extends HistoryItemContents
    with TableInfo<$HistoryItemContentsTable, HistoryItemContent> {
  @override
  final GeneratedDatabase attachedDatabase;
  final String? _alias;
  $HistoryItemContentsTable(this.attachedDatabase, [this._alias]);
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
  static const VerificationMeta _itemIdMeta = const VerificationMeta('itemId');
  @override
  late final GeneratedColumn<int> itemId = GeneratedColumn<int>(
    'item_id',
    aliasedName,
    false,
    type: DriftSqlType.int,
    requiredDuringInsert: true,
    defaultConstraints: GeneratedColumn.constraintIsAlways(
      'REFERENCES history_items (id) ON DELETE CASCADE',
    ),
  );
  static const VerificationMeta _typeMeta = const VerificationMeta('type');
  @override
  late final GeneratedColumn<String> type = GeneratedColumn<String>(
    'type',
    aliasedName,
    false,
    type: DriftSqlType.string,
    requiredDuringInsert: true,
  );
  static const VerificationMeta _valueMeta = const VerificationMeta('value');
  @override
  late final GeneratedColumn<Uint8List> value = GeneratedColumn<Uint8List>(
    'value',
    aliasedName,
    true,
    type: DriftSqlType.blob,
    requiredDuringInsert: false,
  );
  @override
  List<GeneratedColumn> get $columns => [id, itemId, type, value];
  @override
  String get aliasedName => _alias ?? actualTableName;
  @override
  String get actualTableName => $name;
  static const String $name = 'history_item_contents';
  @override
  VerificationContext validateIntegrity(
    Insertable<HistoryItemContent> instance, {
    bool isInserting = false,
  }) {
    final context = VerificationContext();
    final data = instance.toColumns(true);
    if (data.containsKey('id')) {
      context.handle(_idMeta, id.isAcceptableOrUnknown(data['id']!, _idMeta));
    }
    if (data.containsKey('item_id')) {
      context.handle(
        _itemIdMeta,
        itemId.isAcceptableOrUnknown(data['item_id']!, _itemIdMeta),
      );
    } else if (isInserting) {
      context.missing(_itemIdMeta);
    }
    if (data.containsKey('type')) {
      context.handle(
        _typeMeta,
        type.isAcceptableOrUnknown(data['type']!, _typeMeta),
      );
    } else if (isInserting) {
      context.missing(_typeMeta);
    }
    if (data.containsKey('value')) {
      context.handle(
        _valueMeta,
        value.isAcceptableOrUnknown(data['value']!, _valueMeta),
      );
    }
    return context;
  }

  @override
  Set<GeneratedColumn> get $primaryKey => {id};
  @override
  HistoryItemContent map(Map<String, dynamic> data, {String? tablePrefix}) {
    final effectivePrefix = tablePrefix != null ? '$tablePrefix.' : '';
    return HistoryItemContent(
      id: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}id'],
      )!,
      itemId: attachedDatabase.typeMapping.read(
        DriftSqlType.int,
        data['${effectivePrefix}item_id'],
      )!,
      type: attachedDatabase.typeMapping.read(
        DriftSqlType.string,
        data['${effectivePrefix}type'],
      )!,
      value: attachedDatabase.typeMapping.read(
        DriftSqlType.blob,
        data['${effectivePrefix}value'],
      ),
    );
  }

  @override
  $HistoryItemContentsTable createAlias(String alias) {
    return $HistoryItemContentsTable(attachedDatabase, alias);
  }
}

class HistoryItemContent extends DataClass
    implements Insertable<HistoryItemContent> {
  final int id;
  final int itemId;
  final String type;
  final Uint8List? value;
  const HistoryItemContent({
    required this.id,
    required this.itemId,
    required this.type,
    this.value,
  });
  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    map['id'] = Variable<int>(id);
    map['item_id'] = Variable<int>(itemId);
    map['type'] = Variable<String>(type);
    if (!nullToAbsent || value != null) {
      map['value'] = Variable<Uint8List>(value);
    }
    return map;
  }

  HistoryItemContentsCompanion toCompanion(bool nullToAbsent) {
    return HistoryItemContentsCompanion(
      id: Value(id),
      itemId: Value(itemId),
      type: Value(type),
      value: value == null && nullToAbsent
          ? const Value.absent()
          : Value(value),
    );
  }

  factory HistoryItemContent.fromJson(
    Map<String, dynamic> json, {
    ValueSerializer? serializer,
  }) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return HistoryItemContent(
      id: serializer.fromJson<int>(json['id']),
      itemId: serializer.fromJson<int>(json['itemId']),
      type: serializer.fromJson<String>(json['type']),
      value: serializer.fromJson<Uint8List?>(json['value']),
    );
  }
  @override
  Map<String, dynamic> toJson({ValueSerializer? serializer}) {
    serializer ??= driftRuntimeOptions.defaultSerializer;
    return <String, dynamic>{
      'id': serializer.toJson<int>(id),
      'itemId': serializer.toJson<int>(itemId),
      'type': serializer.toJson<String>(type),
      'value': serializer.toJson<Uint8List?>(value),
    };
  }

  HistoryItemContent copyWith({
    int? id,
    int? itemId,
    String? type,
    Value<Uint8List?> value = const Value.absent(),
  }) => HistoryItemContent(
    id: id ?? this.id,
    itemId: itemId ?? this.itemId,
    type: type ?? this.type,
    value: value.present ? value.value : this.value,
  );
  HistoryItemContent copyWithCompanion(HistoryItemContentsCompanion data) {
    return HistoryItemContent(
      id: data.id.present ? data.id.value : this.id,
      itemId: data.itemId.present ? data.itemId.value : this.itemId,
      type: data.type.present ? data.type.value : this.type,
      value: data.value.present ? data.value.value : this.value,
    );
  }

  @override
  String toString() {
    return (StringBuffer('HistoryItemContent(')
          ..write('id: $id, ')
          ..write('itemId: $itemId, ')
          ..write('type: $type, ')
          ..write('value: $value')
          ..write(')'))
        .toString();
  }

  @override
  int get hashCode =>
      Object.hash(id, itemId, type, $driftBlobEquality.hash(value));
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      (other is HistoryItemContent &&
          other.id == this.id &&
          other.itemId == this.itemId &&
          other.type == this.type &&
          $driftBlobEquality.equals(other.value, this.value));
}

class HistoryItemContentsCompanion extends UpdateCompanion<HistoryItemContent> {
  final Value<int> id;
  final Value<int> itemId;
  final Value<String> type;
  final Value<Uint8List?> value;
  const HistoryItemContentsCompanion({
    this.id = const Value.absent(),
    this.itemId = const Value.absent(),
    this.type = const Value.absent(),
    this.value = const Value.absent(),
  });
  HistoryItemContentsCompanion.insert({
    this.id = const Value.absent(),
    required int itemId,
    required String type,
    this.value = const Value.absent(),
  }) : itemId = Value(itemId),
       type = Value(type);
  static Insertable<HistoryItemContent> custom({
    Expression<int>? id,
    Expression<int>? itemId,
    Expression<String>? type,
    Expression<Uint8List>? value,
  }) {
    return RawValuesInsertable({
      if (id != null) 'id': id,
      if (itemId != null) 'item_id': itemId,
      if (type != null) 'type': type,
      if (value != null) 'value': value,
    });
  }

  HistoryItemContentsCompanion copyWith({
    Value<int>? id,
    Value<int>? itemId,
    Value<String>? type,
    Value<Uint8List?>? value,
  }) {
    return HistoryItemContentsCompanion(
      id: id ?? this.id,
      itemId: itemId ?? this.itemId,
      type: type ?? this.type,
      value: value ?? this.value,
    );
  }

  @override
  Map<String, Expression> toColumns(bool nullToAbsent) {
    final map = <String, Expression>{};
    if (id.present) {
      map['id'] = Variable<int>(id.value);
    }
    if (itemId.present) {
      map['item_id'] = Variable<int>(itemId.value);
    }
    if (type.present) {
      map['type'] = Variable<String>(type.value);
    }
    if (value.present) {
      map['value'] = Variable<Uint8List>(value.value);
    }
    return map;
  }

  @override
  String toString() {
    return (StringBuffer('HistoryItemContentsCompanion(')
          ..write('id: $id, ')
          ..write('itemId: $itemId, ')
          ..write('type: $type, ')
          ..write('value: $value')
          ..write(')'))
        .toString();
  }
}

abstract class _$AppDatabase extends GeneratedDatabase {
  _$AppDatabase(QueryExecutor e) : super(e);
  $AppDatabaseManager get managers => $AppDatabaseManager(this);
  late final $HistoryItemsTable historyItems = $HistoryItemsTable(this);
  late final $HistoryItemContentsTable historyItemContents =
      $HistoryItemContentsTable(this);
  @override
  Iterable<TableInfo<Table, Object?>> get allTables =>
      allSchemaEntities.whereType<TableInfo<Table, Object?>>();
  @override
  List<DatabaseSchemaEntity> get allSchemaEntities => [
    historyItems,
    historyItemContents,
  ];
  @override
  StreamQueryUpdateRules get streamUpdateRules => const StreamQueryUpdateRules([
    WritePropagation(
      on: TableUpdateQuery.onTableName(
        'history_items',
        limitUpdateKind: UpdateKind.delete,
      ),
      result: [TableUpdate('history_item_contents', kind: UpdateKind.delete)],
    ),
  ]);
}

typedef $$HistoryItemsTableCreateCompanionBuilder =
    HistoryItemsCompanion Function({
      Value<int> id,
      Value<String?> application,
      required DateTime firstCopiedAt,
      required DateTime lastCopiedAt,
      Value<int> numberOfCopies,
      Value<String?> pin,
      required String title,
      Value<String?> alias,
    });
typedef $$HistoryItemsTableUpdateCompanionBuilder =
    HistoryItemsCompanion Function({
      Value<int> id,
      Value<String?> application,
      Value<DateTime> firstCopiedAt,
      Value<DateTime> lastCopiedAt,
      Value<int> numberOfCopies,
      Value<String?> pin,
      Value<String> title,
      Value<String?> alias,
    });

final class $$HistoryItemsTableReferences
    extends BaseReferences<_$AppDatabase, $HistoryItemsTable, HistoryItem> {
  $$HistoryItemsTableReferences(super.$_db, super.$_table, super.$_typedResult);

  static MultiTypedResultKey<
    $HistoryItemContentsTable,
    List<HistoryItemContent>
  >
  _historyItemContentsRefsTable(_$AppDatabase db) =>
      MultiTypedResultKey.fromTable(
        db.historyItemContents,
        aliasName: $_aliasNameGenerator(
          db.historyItems.id,
          db.historyItemContents.itemId,
        ),
      );

  $$HistoryItemContentsTableProcessedTableManager get historyItemContentsRefs {
    final manager = $$HistoryItemContentsTableTableManager(
      $_db,
      $_db.historyItemContents,
    ).filter((f) => f.itemId.id.sqlEquals($_itemColumn<int>('id')!));

    final cache = $_typedResult.readTableOrNull(
      _historyItemContentsRefsTable($_db),
    );
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: cache),
    );
  }
}

class $$HistoryItemsTableFilterComposer
    extends Composer<_$AppDatabase, $HistoryItemsTable> {
  $$HistoryItemsTableFilterComposer({
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

  ColumnFilters<String> get application => $composableBuilder(
    column: $table.application,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get firstCopiedAt => $composableBuilder(
    column: $table.firstCopiedAt,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<DateTime> get lastCopiedAt => $composableBuilder(
    column: $table.lastCopiedAt,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<int> get numberOfCopies => $composableBuilder(
    column: $table.numberOfCopies,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get pin => $composableBuilder(
    column: $table.pin,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get title => $composableBuilder(
    column: $table.title,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<String> get alias => $composableBuilder(
    column: $table.alias,
    builder: (column) => ColumnFilters(column),
  );

  Expression<bool> historyItemContentsRefs(
    Expression<bool> Function($$HistoryItemContentsTableFilterComposer f) f,
  ) {
    final $$HistoryItemContentsTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.id,
      referencedTable: $db.historyItemContents,
      getReferencedColumn: (t) => t.itemId,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$HistoryItemContentsTableFilterComposer(
            $db: $db,
            $table: $db.historyItemContents,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return f(composer);
  }
}

class $$HistoryItemsTableOrderingComposer
    extends Composer<_$AppDatabase, $HistoryItemsTable> {
  $$HistoryItemsTableOrderingComposer({
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

  ColumnOrderings<String> get application => $composableBuilder(
    column: $table.application,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get firstCopiedAt => $composableBuilder(
    column: $table.firstCopiedAt,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<DateTime> get lastCopiedAt => $composableBuilder(
    column: $table.lastCopiedAt,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<int> get numberOfCopies => $composableBuilder(
    column: $table.numberOfCopies,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get pin => $composableBuilder(
    column: $table.pin,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get title => $composableBuilder(
    column: $table.title,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<String> get alias => $composableBuilder(
    column: $table.alias,
    builder: (column) => ColumnOrderings(column),
  );
}

class $$HistoryItemsTableAnnotationComposer
    extends Composer<_$AppDatabase, $HistoryItemsTable> {
  $$HistoryItemsTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  GeneratedColumn<int> get id =>
      $composableBuilder(column: $table.id, builder: (column) => column);

  GeneratedColumn<String> get application => $composableBuilder(
    column: $table.application,
    builder: (column) => column,
  );

  GeneratedColumn<DateTime> get firstCopiedAt => $composableBuilder(
    column: $table.firstCopiedAt,
    builder: (column) => column,
  );

  GeneratedColumn<DateTime> get lastCopiedAt => $composableBuilder(
    column: $table.lastCopiedAt,
    builder: (column) => column,
  );

  GeneratedColumn<int> get numberOfCopies => $composableBuilder(
    column: $table.numberOfCopies,
    builder: (column) => column,
  );

  GeneratedColumn<String> get pin =>
      $composableBuilder(column: $table.pin, builder: (column) => column);

  GeneratedColumn<String> get title =>
      $composableBuilder(column: $table.title, builder: (column) => column);

  GeneratedColumn<String> get alias =>
      $composableBuilder(column: $table.alias, builder: (column) => column);

  Expression<T> historyItemContentsRefs<T extends Object>(
    Expression<T> Function($$HistoryItemContentsTableAnnotationComposer a) f,
  ) {
    final $$HistoryItemContentsTableAnnotationComposer composer =
        $composerBuilder(
          composer: this,
          getCurrentColumn: (t) => t.id,
          referencedTable: $db.historyItemContents,
          getReferencedColumn: (t) => t.itemId,
          builder:
              (
                joinBuilder, {
                $addJoinBuilderToRootComposer,
                $removeJoinBuilderFromRootComposer,
              }) => $$HistoryItemContentsTableAnnotationComposer(
                $db: $db,
                $table: $db.historyItemContents,
                $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
                joinBuilder: joinBuilder,
                $removeJoinBuilderFromRootComposer:
                    $removeJoinBuilderFromRootComposer,
              ),
        );
    return f(composer);
  }
}

class $$HistoryItemsTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $HistoryItemsTable,
          HistoryItem,
          $$HistoryItemsTableFilterComposer,
          $$HistoryItemsTableOrderingComposer,
          $$HistoryItemsTableAnnotationComposer,
          $$HistoryItemsTableCreateCompanionBuilder,
          $$HistoryItemsTableUpdateCompanionBuilder,
          (HistoryItem, $$HistoryItemsTableReferences),
          HistoryItem,
          PrefetchHooks Function({bool historyItemContentsRefs})
        > {
  $$HistoryItemsTableTableManager(_$AppDatabase db, $HistoryItemsTable table)
    : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$HistoryItemsTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$HistoryItemsTableOrderingComposer($db: db, $table: table),
          createComputedFieldComposer: () =>
              $$HistoryItemsTableAnnotationComposer($db: db, $table: table),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<String?> application = const Value.absent(),
                Value<DateTime> firstCopiedAt = const Value.absent(),
                Value<DateTime> lastCopiedAt = const Value.absent(),
                Value<int> numberOfCopies = const Value.absent(),
                Value<String?> pin = const Value.absent(),
                Value<String> title = const Value.absent(),
                Value<String?> alias = const Value.absent(),
              }) => HistoryItemsCompanion(
                id: id,
                application: application,
                firstCopiedAt: firstCopiedAt,
                lastCopiedAt: lastCopiedAt,
                numberOfCopies: numberOfCopies,
                pin: pin,
                title: title,
                alias: alias,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<String?> application = const Value.absent(),
                required DateTime firstCopiedAt,
                required DateTime lastCopiedAt,
                Value<int> numberOfCopies = const Value.absent(),
                Value<String?> pin = const Value.absent(),
                required String title,
                Value<String?> alias = const Value.absent(),
              }) => HistoryItemsCompanion.insert(
                id: id,
                application: application,
                firstCopiedAt: firstCopiedAt,
                lastCopiedAt: lastCopiedAt,
                numberOfCopies: numberOfCopies,
                pin: pin,
                title: title,
                alias: alias,
              ),
          withReferenceMapper: (p0) => p0
              .map(
                (e) => (
                  e.readTable(table),
                  $$HistoryItemsTableReferences(db, table, e),
                ),
              )
              .toList(),
          prefetchHooksCallback: ({historyItemContentsRefs = false}) {
            return PrefetchHooks(
              db: db,
              explicitlyWatchedTables: [
                if (historyItemContentsRefs) db.historyItemContents,
              ],
              addJoins: null,
              getPrefetchedDataCallback: (items) async {
                return [
                  if (historyItemContentsRefs)
                    await $_getPrefetchedData<
                      HistoryItem,
                      $HistoryItemsTable,
                      HistoryItemContent
                    >(
                      currentTable: table,
                      referencedTable: $$HistoryItemsTableReferences
                          ._historyItemContentsRefsTable(db),
                      managerFromTypedResult: (p0) =>
                          $$HistoryItemsTableReferences(
                            db,
                            table,
                            p0,
                          ).historyItemContentsRefs,
                      referencedItemsForCurrentItem: (item, referencedItems) =>
                          referencedItems.where((e) => e.itemId == item.id),
                      typedResults: items,
                    ),
                ];
              },
            );
          },
        ),
      );
}

typedef $$HistoryItemsTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $HistoryItemsTable,
      HistoryItem,
      $$HistoryItemsTableFilterComposer,
      $$HistoryItemsTableOrderingComposer,
      $$HistoryItemsTableAnnotationComposer,
      $$HistoryItemsTableCreateCompanionBuilder,
      $$HistoryItemsTableUpdateCompanionBuilder,
      (HistoryItem, $$HistoryItemsTableReferences),
      HistoryItem,
      PrefetchHooks Function({bool historyItemContentsRefs})
    >;
typedef $$HistoryItemContentsTableCreateCompanionBuilder =
    HistoryItemContentsCompanion Function({
      Value<int> id,
      required int itemId,
      required String type,
      Value<Uint8List?> value,
    });
typedef $$HistoryItemContentsTableUpdateCompanionBuilder =
    HistoryItemContentsCompanion Function({
      Value<int> id,
      Value<int> itemId,
      Value<String> type,
      Value<Uint8List?> value,
    });

final class $$HistoryItemContentsTableReferences
    extends
        BaseReferences<
          _$AppDatabase,
          $HistoryItemContentsTable,
          HistoryItemContent
        > {
  $$HistoryItemContentsTableReferences(
    super.$_db,
    super.$_table,
    super.$_typedResult,
  );

  static $HistoryItemsTable _itemIdTable(_$AppDatabase db) =>
      db.historyItems.createAlias(
        $_aliasNameGenerator(db.historyItemContents.itemId, db.historyItems.id),
      );

  $$HistoryItemsTableProcessedTableManager get itemId {
    final $_column = $_itemColumn<int>('item_id')!;

    final manager = $$HistoryItemsTableTableManager(
      $_db,
      $_db.historyItems,
    ).filter((f) => f.id.sqlEquals($_column));
    final item = $_typedResult.readTableOrNull(_itemIdTable($_db));
    if (item == null) return manager;
    return ProcessedTableManager(
      manager.$state.copyWith(prefetchedData: [item]),
    );
  }
}

class $$HistoryItemContentsTableFilterComposer
    extends Composer<_$AppDatabase, $HistoryItemContentsTable> {
  $$HistoryItemContentsTableFilterComposer({
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

  ColumnFilters<String> get type => $composableBuilder(
    column: $table.type,
    builder: (column) => ColumnFilters(column),
  );

  ColumnFilters<Uint8List> get value => $composableBuilder(
    column: $table.value,
    builder: (column) => ColumnFilters(column),
  );

  $$HistoryItemsTableFilterComposer get itemId {
    final $$HistoryItemsTableFilterComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.itemId,
      referencedTable: $db.historyItems,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$HistoryItemsTableFilterComposer(
            $db: $db,
            $table: $db.historyItems,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$HistoryItemContentsTableOrderingComposer
    extends Composer<_$AppDatabase, $HistoryItemContentsTable> {
  $$HistoryItemContentsTableOrderingComposer({
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

  ColumnOrderings<String> get type => $composableBuilder(
    column: $table.type,
    builder: (column) => ColumnOrderings(column),
  );

  ColumnOrderings<Uint8List> get value => $composableBuilder(
    column: $table.value,
    builder: (column) => ColumnOrderings(column),
  );

  $$HistoryItemsTableOrderingComposer get itemId {
    final $$HistoryItemsTableOrderingComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.itemId,
      referencedTable: $db.historyItems,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$HistoryItemsTableOrderingComposer(
            $db: $db,
            $table: $db.historyItems,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$HistoryItemContentsTableAnnotationComposer
    extends Composer<_$AppDatabase, $HistoryItemContentsTable> {
  $$HistoryItemContentsTableAnnotationComposer({
    required super.$db,
    required super.$table,
    super.joinBuilder,
    super.$addJoinBuilderToRootComposer,
    super.$removeJoinBuilderFromRootComposer,
  });
  GeneratedColumn<int> get id =>
      $composableBuilder(column: $table.id, builder: (column) => column);

  GeneratedColumn<String> get type =>
      $composableBuilder(column: $table.type, builder: (column) => column);

  GeneratedColumn<Uint8List> get value =>
      $composableBuilder(column: $table.value, builder: (column) => column);

  $$HistoryItemsTableAnnotationComposer get itemId {
    final $$HistoryItemsTableAnnotationComposer composer = $composerBuilder(
      composer: this,
      getCurrentColumn: (t) => t.itemId,
      referencedTable: $db.historyItems,
      getReferencedColumn: (t) => t.id,
      builder:
          (
            joinBuilder, {
            $addJoinBuilderToRootComposer,
            $removeJoinBuilderFromRootComposer,
          }) => $$HistoryItemsTableAnnotationComposer(
            $db: $db,
            $table: $db.historyItems,
            $addJoinBuilderToRootComposer: $addJoinBuilderToRootComposer,
            joinBuilder: joinBuilder,
            $removeJoinBuilderFromRootComposer:
                $removeJoinBuilderFromRootComposer,
          ),
    );
    return composer;
  }
}

class $$HistoryItemContentsTableTableManager
    extends
        RootTableManager<
          _$AppDatabase,
          $HistoryItemContentsTable,
          HistoryItemContent,
          $$HistoryItemContentsTableFilterComposer,
          $$HistoryItemContentsTableOrderingComposer,
          $$HistoryItemContentsTableAnnotationComposer,
          $$HistoryItemContentsTableCreateCompanionBuilder,
          $$HistoryItemContentsTableUpdateCompanionBuilder,
          (HistoryItemContent, $$HistoryItemContentsTableReferences),
          HistoryItemContent,
          PrefetchHooks Function({bool itemId})
        > {
  $$HistoryItemContentsTableTableManager(
    _$AppDatabase db,
    $HistoryItemContentsTable table,
  ) : super(
        TableManagerState(
          db: db,
          table: table,
          createFilteringComposer: () =>
              $$HistoryItemContentsTableFilterComposer($db: db, $table: table),
          createOrderingComposer: () =>
              $$HistoryItemContentsTableOrderingComposer(
                $db: db,
                $table: table,
              ),
          createComputedFieldComposer: () =>
              $$HistoryItemContentsTableAnnotationComposer(
                $db: db,
                $table: table,
              ),
          updateCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                Value<int> itemId = const Value.absent(),
                Value<String> type = const Value.absent(),
                Value<Uint8List?> value = const Value.absent(),
              }) => HistoryItemContentsCompanion(
                id: id,
                itemId: itemId,
                type: type,
                value: value,
              ),
          createCompanionCallback:
              ({
                Value<int> id = const Value.absent(),
                required int itemId,
                required String type,
                Value<Uint8List?> value = const Value.absent(),
              }) => HistoryItemContentsCompanion.insert(
                id: id,
                itemId: itemId,
                type: type,
                value: value,
              ),
          withReferenceMapper: (p0) => p0
              .map(
                (e) => (
                  e.readTable(table),
                  $$HistoryItemContentsTableReferences(db, table, e),
                ),
              )
              .toList(),
          prefetchHooksCallback: ({itemId = false}) {
            return PrefetchHooks(
              db: db,
              explicitlyWatchedTables: [],
              addJoins:
                  <
                    T extends TableManagerState<
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic,
                      dynamic
                    >
                  >(state) {
                    if (itemId) {
                      state =
                          state.withJoin(
                                currentTable: table,
                                currentColumn: table.itemId,
                                referencedTable:
                                    $$HistoryItemContentsTableReferences
                                        ._itemIdTable(db),
                                referencedColumn:
                                    $$HistoryItemContentsTableReferences
                                        ._itemIdTable(db)
                                        .id,
                              )
                              as T;
                    }

                    return state;
                  },
              getPrefetchedDataCallback: (items) async {
                return [];
              },
            );
          },
        ),
      );
}

typedef $$HistoryItemContentsTableProcessedTableManager =
    ProcessedTableManager<
      _$AppDatabase,
      $HistoryItemContentsTable,
      HistoryItemContent,
      $$HistoryItemContentsTableFilterComposer,
      $$HistoryItemContentsTableOrderingComposer,
      $$HistoryItemContentsTableAnnotationComposer,
      $$HistoryItemContentsTableCreateCompanionBuilder,
      $$HistoryItemContentsTableUpdateCompanionBuilder,
      (HistoryItemContent, $$HistoryItemContentsTableReferences),
      HistoryItemContent,
      PrefetchHooks Function({bool itemId})
    >;

class $AppDatabaseManager {
  final _$AppDatabase _db;
  $AppDatabaseManager(this._db);
  $$HistoryItemsTableTableManager get historyItems =>
      $$HistoryItemsTableTableManager(_db, _db.historyItems);
  $$HistoryItemContentsTableTableManager get historyItemContents =>
      $$HistoryItemContentsTableTableManager(_db, _db.historyItemContents);
}
