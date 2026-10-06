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
  - 存档 AshenDepths1.sav ~ AshenDepths3.sav 保存在 exe 同一目录。

"""

GUIDE_CALC = """【计算器版】TI-Nspire CX / CX CAS（需先安装 Ndless）
  用 TI-Nspire Computer Link 把 AshenDepths.tns 拷进 ndless 文件夹，
  在"我的文档"里打开即可。存档 AshenDepths1.sav.tns 等会出现在同一目录。

Desktop: double-click AshenDepthsDesktop.exe (Space toggles auto battle).
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
