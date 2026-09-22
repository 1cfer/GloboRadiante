"""Pruebas locales de ecuacion real y estructura; NO compila el firmware ESP32."""
from pathlib import Path
import hashlib
import math
import re
import subprocess
import tempfile
import json

ROOT = Path(__file__).resolve().parent
SKETCH = ROOT / "main"
source = (SKETCH / "SensorManager.cpp").read_text()
match = re.search(r"float SensorManager::calculateMRT\(float tg, float ta\) \{.*?\n\}", source, re.S)
assert match, "No se encontro la funcion MRT"
function = match.group(0).replace("SensorManager::", "")
program = '#include <cmath>\n#include <cstdio>\nusing namespace std;\n' + function
program += '\nint main(){float tg,ta;while(scanf("%f %f",&tg,&ta)==2)printf("%.9g\\n",calculateMRT(tg,ta));}\n'
cases = [(25,25),(30,25),(20,25),(-10,-10),(0,20),(55,30),(85,25),(-274,25),(float("nan"),25)]
cases += [(tg,ta) for tg in range(-20,61,5) for ta in range(-20,61,5)]
with tempfile.TemporaryDirectory(prefix="globo-test-") as tmp:
    cpp = Path(tmp) / "mrt.cpp"
    binary = Path(tmp) / "mrt_test"
    cpp.write_text(program)
    subprocess.run(["g++","-std=c++11","-Wall","-Wextra","-Werror",str(cpp),"-o",str(binary)],check=True)
    result = subprocess.run([str(binary)],input="".join(f"{g} {a}\n" for g,a in cases),text=True,capture_output=True,check=True)
    values = list(map(float,result.stdout.split()))
    assert len(values) == len(cases)
    for (tg,ta),actual in zip(cases,values):
        if not math.isfinite(tg) or tg <= -273.15 or ta <= -273.15:
            assert math.isnan(actual)
            continue
        radicand = (tg+273.15)**4+4e7*abs(tg-ta)**0.25*(tg-ta)
        if radicand <= 0:
            assert math.isnan(actual)
            continue
        expected = radicand**0.25-273.15
        assert math.isclose(actual,expected,abs_tol=1e-5), (tg,ta,actual,expected)
    print(f"PASS: {len(cases)} casos numericos de la funcion C++ real")
    for pair,value in zip(cases[:6],values[:6]):
        print(f"Tg={pair[0]}, Ta={pair[1]} -> MRT={value:.6f} C")

expected_hashes = json.loads((ROOT / "base_sha256.json").read_text())
for name,digest in expected_hashes.items():
    assert hashlib.sha256((SKETCH / name).read_bytes()).hexdigest() == digest, name
print(f"PASS: {len(expected_hashes)} archivos base intactos (SHA-256)")
states = (SKETCH / "Estados.cpp").read_text()
for key in ("tempAire","tempGlobo","mrt","humedad"):
    assert f'doc["{key}"]["type"] = "Float";' in states
    assert f'doc["{key}"]["value"] = avg.{key};' in states
exit_body = states.split("void EstadoDESARROLLADOR::onExit()",1)[1]
assert "WiFi.enableAP(false)" in exit_body and "captiveDns.stop()" in exit_body
assert 'firstRun = false;\n    statemachine->ChangeState(new EstadoLECTURA());' in states
config = (SKETCH / "AppConfig.h").read_text()
assert "GLOBE_PIN = 27" in config and "BUTTON_PIN = 0" in config
assert 'hostname = "globo-mrt-01"' in (SKETCH / "AppConfig.cpp").read_text()
assert list(SKETCH.glob("*.ino")) == [SKETCH / "main.ino"]
print("PASS: payload, pines, hostname, salida AP y estructura Arduino")
print("PENDIENTE: compilacion Arduino/ESP32, pruebas en placa, HTTP y OTA reales")
