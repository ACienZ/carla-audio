python3 - <<'PY'
import json
import shlex
import shutil
from pathlib import Path

vscode = Path("/home/test/carla-0.9.16/Unreal/CarlaUE4/.vscode")
engine = Path("/home/test/UnrealEngine_4.26/Engine")

source_db = vscode / "compileCommands_CarlaUE4.json"
fixed_db = vscode / "compileCommands_CarlaUE4_intellisense.json"
properties = vscode / "c_cpp_properties.json"

rsp = vscode / "compileCommands_CarlaUE4/Carla.1.rsp"

sysroot = (
    engine
    / "Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64"
    / "v17_clang-10.0.1-centos7/x86_64-unknown-linux-gnu"
)
compiler = sysroot / "bin/clang++"
libcxx = engine / "Source/ThirdParty/Linux/LibCxx/include/c++/v1"

# 写入前先确认输入文件和工具链存在。
for path in (
    source_db, properties, rsp, compiler,
    libcxx / "cstdint",
    libcxx / "array",
    libcxx / "initializer_list",
):
    if not path.is_file():
        raise SystemExit(f"文件不存在，停止修改：{path}")

entries = json.loads(source_db.read_text())
config = json.loads(properties.read_text())

active_name = "CarlaUE4Editor Editor Linux Development (CarlaUE4)"
active = next(
    (item for item in config["configurations"]
     if item["name"] == active_name),
    None,
)
if active is None:
    raise SystemExit("没有找到目标配置，停止修改。")

# 展开响应文件，完整保留现有 include 和强制包含的头文件。
rsp_arguments = shlex.split(rsp.read_text())

changed = 0
for entry in entries:
    original_arguments = entry.get("arguments")
    if original_arguments is None:
        original_arguments = shlex.split(entry.get("command", ""))

    # 只修复使用这份 Carla 模块响应文件的记录。
    if "@" + str(rsp) not in original_arguments:
        continue

    entry["arguments"] = [
        str(compiler),
        "-x", "c++",
        "--target=x86_64-unknown-linux-gnu",
        "-m64",
        "--sysroot=" + str(sysroot),
        "-stdlib=libc++",
        "-isystem", str(libcxx),
        *rsp_arguments,
        "-c", entry["file"],
    ]
    entry.pop("command", None)
    changed += 1

if changed == 0:
    raise SystemExit("没有匹配到 Carla.1.rsp 的记录，停止修改。")

# 保留第一次修改前的配置，重复运行不会覆盖这份备份。
backup = properties.with_name("c_cpp_properties.before-libcxx-fix.json")
if not backup.exists():
    shutil.copy2(properties, backup)

fixed_db.write_text(json.dumps(entries, indent=2) + "\n")

active["compileCommands"] = [str(fixed_db)]
active["compilerPath"] = str(compiler)
active["intelliSenseMode"] = "linux-clang-x64"

properties.write_text(json.dumps(config, indent=2) + "\n")

print(f"已修复 {changed} 条 Carla 模块的 IntelliSense 记录")
print(f"新数据库：{fixed_db}")
print(f"配置备份：{backup}")
PY