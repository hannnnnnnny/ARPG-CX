"""Pack dist/ into the one-click download zip attached to GitHub releases.

    python tools/package_release.py        -> dist/AshenDepths-Windows.zip
                                              (or AshenDepths-TI-Nspire.zip
                                              when there is no desktop exe)

The zip name stays fixed so the README can link to
releases/latest/download/<name> and keep working across releases.
"""
import pathlib
import sys
import zipfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
DIST = ROOT / "dist"
EXE = DIST / "AshenDepthsDesktop.exe"
TNS = DIST / "AshenDepths.tns"

GUIDE_PC = """ASHEN DEPTHS 灰烬深渊
=====================

【电脑版】双击 AshenDepthsDesktop.exe 即可游玩，无需安装。
  - Windows 可能提示"已保护你的电脑"：点"更多信息" -> "仍要运行"。
  - 空格：自动挂机 / 亲自操作切换；WASD 移动；鼠标点怪攻击；1-6 技能；Q 喝药。
  - F11 或 Alt+Enter：全屏（宽屏两侧显示小地图、流派、战利品和本层事件）。
  - 支持 Xbox 手柄；改键请编辑 exe 旁边自动生成的 settings.ini。
  - 选项里可以分别调节音效和背景音乐的音量。
  - 如果游戏崩溃，exe 旁边会生成 crash.txt，请把它附在 GitHub issue 里。
  - 15 个存档位，存档文件保存在 exe 同一目录。
  - 请先解压到普通文件夹（如桌面）再运行；不要直接在压缩包里双击，也不要放进 Program Files，否则存档可能保存失败。
  - 需要 Windows 10/11（Windows 7/8.1 需先安装微软 UCRT 运行库更新 KB2999226）。

"""

GUIDE_CALC = """【计算器版】TI-Nspire CX / CX CAS（需先安装 Ndless）
  用 TI-Nspire Computer Link 把 AshenDepths.tns 拷进 ndless 文件夹，
  在"我的文档"里打开即可。存档 AshenDepths1.sav.tns 等会出现在同一目录。

Desktop: double-click AshenDepthsDesktop.exe (Space toggles auto battle).
  F11 / Alt+Enter: fullscreen with side panels. Xbox gamepads work; rebind
  keys in settings.ini next to the exe. Crashes leave a crash.txt to report.
Calculator: copy AshenDepths.tns into the ndless folder (Ndless required).
Fonts: Fusion Pixel / Boutique Bitmap / Galmuri etc., licenses in fonts/.
"""


def license_files():
    """Every font license shipped in assets/, as (path on disk, path in zip)."""
    for base in ("fonts", "fonts12"):
        src = ROOT / "assets" / base
        for f in sorted(src.rglob("*")):
            if f.is_file() and f.suffix.lower() in (".txt", ".md"):
                yield f, "fonts/" + base + "/" + f.relative_to(src).as_posix()


def main():
    if not TNS.is_file():
        sys.exit("dist/AshenDepths.tns missing: run tools/build_nspire.sh first")
    desktop = EXE.is_file()
    name = "AshenDepths-Windows.zip" if desktop else "AshenDepths-TI-Nspire.zip"
    guide = (GUIDE_PC if desktop else "") + GUIDE_CALC
    out = DIST / name
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        if desktop:
            z.write(EXE, "AshenDepths/AshenDepthsDesktop.exe")
        z.write(TNS, "AshenDepths/AshenDepths.tns")
        z.writestr("AshenDepths/README-说明.txt", guide.replace("\n", "\r\n"))
        z.write(ROOT / "LICENSE", "AshenDepths/LICENSE.txt")
        for src, arc in license_files():
            z.write(src, "AshenDepths/" + arc)
    print(out, out.stat().st_size)


if __name__ == "__main__":
    main()
