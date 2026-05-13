import 'package:flutter/widgets.dart';

class NavigateDownIntent extends Intent {
  const NavigateDownIntent();
}

class NavigateUpIntent extends Intent {
  const NavigateUpIntent();
}

class SelectItemIntent extends Intent {
  const SelectItemIntent();
}

class CloseWindowIntent extends Intent {
  const CloseWindowIntent();
}

class TogglePinIntent extends Intent {
  const TogglePinIntent();
}

class OpenSettingsIntent extends Intent {
  const OpenSettingsIntent();
}

class QuitAppIntent extends Intent {
  const QuitAppIntent();
}

class QuickSelectIntent extends Intent {
  const QuickSelectIntent(this.index);
  final int index;
}
