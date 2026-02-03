import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_hooks/flutter_hooks.dart';

/// 数字步进器组件，支持手动输入和增减按钮

class NumberStepper extends HookWidget {
  /// 当前数值

  final int value;

  /// 步进值

  final int step;

  /// 最小值

  final int min;

  /// 最大值

  final int max;

  /// 数值变化回调

  final ValueChanged<int> onChanged;

  /// 构造函数

  const NumberStepper({super.key, required this.value, this.step = 1, this.min = 0, this.max = 9999, required this.onChanged});

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    final color = isDark ? Colors.white.withOpacity(0.06) : Colors.black.withOpacity(0.04);

    final borderColor = isDark ? Colors.white.withOpacity(0.1) : Colors.black.withOpacity(0.1);

    final controller = useTextEditingController(text: value.toString());

    final focusNode = useFocusNode();

    // 当外部数值发生变化且当前没有焦点时，同步输入框内容

    useEffect(() {
      if (!focusNode.hasFocus) {
        controller.text = value.toString();
      }

      return null;
    }, [value]);

    /// 验证并提交输入内容

    void validateAndSubmit(String text) {
      final newValue = int.tryParse(text);

      if (newValue != null && newValue >= min && newValue <= max) {
        onChanged(newValue);
      } else {
        // 非法输入，恢复为当前有效值

        controller.text = value.toString();
      }
    }

    return Container(
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

                style: const TextStyle(fontSize: 13, fontFamily: '.AppleSystemUIFont'),

                decoration: null,

                // 去除内边框，统一使用容器边框
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

/// 步进器按钮组件（增/减）

class _StepperButton extends StatelessWidget {
  /// 图标数据

  final IconData icon;

  /// 点击回调

  final VoidCallback onTap;

  /// 构造函数

  const _StepperButton({required this.icon, required this.onTap});

  @override
  Widget build(BuildContext context) {
    final isDark = MediaQuery.of(context).platformBrightness == Brightness.dark;

    return GestureDetector(
      onTap: onTap,

      child: Container(
        width: 24,

        height: 12,

        color: Colors.transparent,

        child: Icon(icon, size: 8, color: isDark ? Colors.white54 : Colors.black54),
      ),
    );
  }
}
