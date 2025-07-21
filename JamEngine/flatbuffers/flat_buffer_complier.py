import os
import subprocess
from pathlib import Path
import tkinter as tk
from tkinter import filedialog

SCRIPT_DIR        = Path(__file__).resolve().parent
FLATC_EXE         = SCRIPT_DIR / "flatc.exe"    
GENERATOR_OPTIONS = "--cpp"                  
OUTPUT_DIRECTORY  = SCRIPT_DIR / "compiled"    

def create_command(input_path: Path) -> list[str]:
    include_dir = input_path.parent
    return [
        str(FLATC_EXE),
        GENERATOR_OPTIONS,
        "-o", str(OUTPUT_DIRECTORY),
        "-I", str(include_dir),
        str(input_path)
    ]

def compile_schemas(schema_paths: list[Path]):
    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)

    for fbs in schema_paths:
        cmd = create_command(fbs)
        print(f"▶ 컴파일: {' '.join(cmd)}")
        try:
            result = subprocess.run(cmd, capture_output=True, text=True)
        except FileNotFoundError:
            print(f"flatc 실행 파일을 찾을 수 없습니다: {FLATC_EXE}")
            return

        if result.returncode != 0:
            print(f"오류 발생: {fbs.name}")
            print(result.stderr)
        else:
            print(f"성공: {fbs.name}")

def select_schema_files() -> list[Path]:
    root = tk.Tk()
    root.withdraw()
    filepaths = filedialog.askopenfilenames(
        title="FlatBuffers 스키마 파일 선택",
        filetypes=[("FlatBuffers Schema","*.fbs")],
    )
    root.destroy()
    return [Path(p) for p in filepaths]

if __name__ == "__main__":
    schemas = select_schema_files()
    if not schemas:
        print("선택된 파일이 없습니다. 종료합니다.")
    else:
        compile_schemas(schemas)
    os.system("pause")