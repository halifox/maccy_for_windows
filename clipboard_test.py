import argparse
import io
import time
import uuid

import win32clipboard
from PIL import Image, ImageDraw


def set_text(text: str):
    win32clipboard.OpenClipboard()

    try:
        win32clipboard.EmptyClipboard()
        win32clipboard.SetClipboardText(
            text,
            win32clipboard.CF_UNICODETEXT,
        )
    finally:
        win32clipboard.CloseClipboard()


def set_image(image: Image.Image):
    output = io.BytesIO()

    image.convert("RGB").save(output, "BMP")

    # CF_DIB 不需要 BMP 文件头，去掉前 14 字节
    data = output.getvalue()[14:]

    output.close()

    win32clipboard.OpenClipboard()

    try:
        win32clipboard.EmptyClipboard()
        win32clipboard.SetClipboardData(
            win32clipboard.CF_DIB,
            data,
        )
    finally:
        win32clipboard.CloseClipboard()


def create_short_text(index: int):
    return f"clipboard-short-{index}-{uuid.uuid4()}"


def create_long_text(index: int, size_kb: int):
    header = f"clipboard-long-{index}-{uuid.uuid4()}\n"

    target_size = size_kb * 1024

    pattern = (
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "测试剪贴板长文本\n"
    )

    repeat_count = target_size // len(pattern.encode("utf-8")) + 1

    body = (pattern * repeat_count).encode("utf-8")[
        :target_size
    ].decode(
        "utf-8",
        errors="ignore",
    )

    return header + body


def create_image(index: int, width: int, height: int):
    image = Image.new(
        "RGB",
        (width, height),
        "white",
    )

    draw = ImageDraw.Draw(image)

    unique_id = str(uuid.uuid4())

    text = (
        f"Clipboard Image\n"
        f"Index: {index}\n"
        f"ID: {unique_id}\n"
        f"Size: {width}x{height}"
    )

    draw.multiline_text(
        (40, 40),
        text,
        fill="black",
        spacing=10,
    )

    # 增加一个由序号决定的位置标记，
    # 保证图片像素本身也不同。
    x = index % max(width, 1)
    y = (index * 17) % max(height, 1)

    draw.rectangle(
        (
            x,
            y,
            min(x + 20, width - 1),
            min(y + 20, height - 1),
        ),
        fill="black",
    )

    return image


def wait(interval_ms: int):
    if interval_ms > 0:
        time.sleep(interval_ms / 1000)


def run_short(args):
    print(
        f"开始短文本测试: "
        f"count={args.count}, "
        f"interval={args.interval}ms"
    )

    for i in range(1, args.count + 1):
        text = create_short_text(i)

        set_text(text)

        print(
            f"[{i}/{args.count}] "
            f"short "
            f"length={len(text)}"
        )

        wait(args.interval)


def run_long(args):
    print(
        f"开始长文本测试: "
        f"count={args.count}, "
        f"size={args.size}KB, "
        f"interval={args.interval}ms"
    )

    for i in range(1, args.count + 1):
        text = create_long_text(
            i,
            args.size,
        )

        set_text(text)

        print(
            f"[{i}/{args.count}] "
            f"long "
            f"chars={len(text)}"
        )

        wait(args.interval)


def run_image(args):
    print(
        f"开始图片测试: "
        f"count={args.count}, "
        f"size={args.width}x{args.height}, "
        f"interval={args.interval}ms"
    )

    for i in range(1, args.count + 1):
        image = create_image(
            i,
            args.width,
            args.height,
        )

        set_image(image)

        image.close()

        print(
            f"[{i}/{args.count}] "
            f"image "
            f"{args.width}x{args.height}"
        )

        wait(args.interval)


def run_mixed(args):
    print(
        f"开始混合测试: "
        f"count={args.count}, "
        f"interval={args.interval}ms"
    )

    for i in range(1, args.count + 1):
        mode = (i - 1) % 3

        if mode == 0:
            text = create_short_text(i)

            set_text(text)

            data_type = "short"

        elif mode == 1:
            text = create_long_text(
                i,
                args.size,
            )

            set_text(text)

            data_type = "long"

        else:
            image = create_image(
                i,
                args.width,
                args.height,
            )

            set_image(image)

            image.close()

            data_type = "image"

        print(
            f"[{i}/{args.count}] "
            f"{data_type}"
        )

        wait(args.interval)


def main():
    parser = argparse.ArgumentParser(
        description="Windows Clipboard Stress Test"
    )

    subparsers = parser.add_subparsers(
        dest="mode",
        required=True,
    )

    short_parser = subparsers.add_parser(
        "short",
        help="复制不重复短文本",
    )

    short_parser.add_argument(
        "--count",
        type=int,
        default=1000,
        help="复制次数，默认 1000",
    )

    short_parser.add_argument(
        "--interval",
        type=int,
        default=50,
        help="每次复制间隔，单位 ms，默认 50",
    )

    long_parser = subparsers.add_parser(
        "long",
        help="复制不重复长文本",
    )

    long_parser.add_argument(
        "--count",
        type=int,
        default=1000,
    )

    long_parser.add_argument(
        "--interval",
        type=int,
        default=100,
    )

    long_parser.add_argument(
        "--size",
        type=int,
        default=50,
        help="每条文本大小，单位 KB，默认 50",
    )

    image_parser = subparsers.add_parser(
        "image",
        help="复制不重复图片",
    )

    image_parser.add_argument(
        "--count",
        type=int,
        default=1000,
    )

    image_parser.add_argument(
        "--interval",
        type=int,
        default=150,
    )

    image_parser.add_argument(
        "--width",
        type=int,
        default=800,
    )

    image_parser.add_argument(
        "--height",
        type=int,
        default=600,
    )

    mixed_parser = subparsers.add_parser(
        "mixed",
        help="短文本、长文本、图片循环复制",
    )

    mixed_parser.add_argument(
        "--count",
        type=int,
        default=3000,
    )

    mixed_parser.add_argument(
        "--interval",
        type=int,
        default=100,
    )

    mixed_parser.add_argument(
        "--size",
        type=int,
        default=50,
        help="长文本大小，单位 KB",
    )

    mixed_parser.add_argument(
        "--width",
        type=int,
        default=800,
    )

    mixed_parser.add_argument(
        "--height",
        type=int,
        default=600,
    )

    args = parser.parse_args()

    start = time.perf_counter()

    try:
        if args.mode == "short":
            run_short(args)

        elif args.mode == "long":
            run_long(args)

        elif args.mode == "image":
            run_image(args)

        elif args.mode == "mixed":
            run_mixed(args)

    except KeyboardInterrupt:
        print("\n测试已停止。")

    finally:
        elapsed = time.perf_counter() - start

        print(
            f"\n耗时: {elapsed:.2f}s"
        )


if __name__ == "__main__":
    main()

# 使用
#
# 1000 个短文本：
#
# py clipboard_test.py short
#
# 等价于：
#
# py clipboard_test.py short --count 1000 --interval 50
#
# 1000 个 50KB 长文本：
#
# py clipboard_test.py long --count 1000 --size 50 --interval 100
#
# 1000 张 800 × 600 图片：
#
# py clipboard_test.py image --count 1000 --width 800 --height 600 --interval 150
#
# 1000 张 1080P 图片：
#
# py clipboard_test.py image --count 1000 --width 1920 --height 1080 --interval 150
#
# 4K：
#
# py clipboard_test.py image --count 1000 --width 3840 --height 2160 --interval 300
#
# 混合测试：
#
# py clipboard_test.py mixed --count 3000 --size 50 --width 800 --height 600 --interval 100
#
# 它会按：
#
# 短文本
# 长文本
# 图片
# 短文本
# 长文本
# 图片
# ...
#
# 循环写入。
#
# 压力测试
#
# 可以逐步降低间隔：
#
# py clipboard_test.py short --count 1000 --interval 100
# py clipboard_test.py short --count 1000 --interval 50
# py clipboard_test.py short --count 1000 --interval 20
# py clipboard_test.py short --count 1000 --interval 10
# py clipboard_test.py short --count 1000 --interval 1
# py clipboard_test.py short --count 1000 --interval 0
#
# 如果你的程序最终只收到：
#
# 997
# 985
# 830
# ...
#
# 就能找到开始丢剪贴板事件的速度。
#
# 另外有一个地方值得测：同内容重复复制。很多剪贴板工具会主动去重。你现在这份脚本刻意让每一条数据不同，所以测的是“1000 次有效新增记录”，不会被去重逻辑干扰。