import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';

/// 数字步进器组件。
///
/// 结合了文本输入框和微调按钮，支持手动键盘输入以及通过上下按钮进行增减。
///
/// 字段说明:
/// [value] 当前展示的数值。
/// [step] 每次点击按钮增减的幅度。
/// [min] 允许输入的最小值。
/// [max] 允许输入的最大值。
/// [onChanged] 数值发生变化且通过校验后的回调。
class NumberStepper extends HookWidget {

  const NumberStepper({
    super.key,
    required this.value,
    this.step = 1,
    this.min = 0,
    this.max = 9999,
    required this.onChanged,
  });
  final int value;
  final int step;
  final int min;
  final int max;
  final ValueChanged<int> onChanged;

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;
    final color = isDark
        ? Colors.white.withValues(alpha: 0.06)
        : Colors.black.withValues(alpha: 0.04);
    final borderColor = isDark
        ? Colors.white.withValues(alpha: 0.1)
        : Colors.black.withValues(alpha: 0.1);
    final controller = useTextEditingController(text: value.toString());
    final focusNode = useFocusNode();

    useEffect(() {
      if (!focusNode.hasFocus) {
        controller.text = value.toString();
      }
      return null;
    }, [value]);

    /// 校验文本输入并提交。
    ///
    /// 若输入非数字或超出边界，将自动恢复为旧值。
    void validateAndSubmit(String text) {
      final newValue = int.tryParse(text);
      if (newValue != null && newValue >= min && newValue <= max) {
        onChanged(newValue);
      } else {
        controller.text = value.toString();
      }
    }

    return DecoratedBox(
      decoration: BoxDecoration(
        color: color,
        borderRadius: BorderRadius.circular(6),
        border: Border.all(color: borderColor, width: 0.5),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          SizedBox(
            width: 50,
            child: Focus(
              onFocusChange: (hasFocus) {
                if (!hasFocus) validateAndSubmit(controller.text);
              },
              child: CupertinoTextField(
                controller: controller,
                focusNode: focusNode,
                textAlign: TextAlign.center,
                padding: const EdgeInsets.symmetric(horizontal: 4),
                style: const TextStyle(
                  fontSize: 13,
                  fontFamily: '.AppleSystemUIFont',
                ),
                decoration: null,
                keyboardType: TextInputType.number,
                onSubmitted: validateAndSubmit,
              ),
            ),
          ),
          Container(width: 0.5, height: 20, color: borderColor),
          Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              _StepperButton(
                icon: CupertinoIcons.chevron_up,
                onTap: () {
                  final next = (value + step).clamp(min, max);
                  onChanged(next);
                  controller.text = next.toString();
                },
              ),
              Container(width: 24, height: 0.5, color: borderColor),
              _StepperButton(
                icon: CupertinoIcons.chevron_down,
                onTap: () {
                  final next = (value - step).clamp(min, max);
                  onChanged(next);
                  controller.text = next.toString();
                },
              ),
            ],
          ),
        ],
      ),
    );
  }
}

/// 步进器内部的小微调按钮。
///
/// 字段说明:
/// [icon] 按钮展示的图标。
/// [onTap] 点击回调。
class _StepperButton extends StatelessWidget {

  const _StepperButton({required this.icon, required this.onTap});
  final IconData icon;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return GestureDetector(
      onTap: onTap,
      child: Container(
        width: 24,
        height: 12,
        color: Colors.transparent,
        child: Icon(
          icon,
          size: 8,
          color: isDark ? Colors.white54 : Colors.black54,
        ),
      ),
    );
  }
}
