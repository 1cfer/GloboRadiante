from pathlib import Path
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, PageBreak
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfbase import pdfmetrics

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "GloboRadiante_Documentacion_Tecnica.pdf"
REPO = "https://github.com/1cfer/GloboRadiante"
pdfmetrics.registerFont(TTFont("DV", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("DVB", "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"))
INK=colors.HexColor("#17313A"); TEAL=colors.HexColor("#287D78")
MUTED=colors.HexColor("#5C6B71"); PALE=colors.HexColor("#EEF6F5"); LINE=colors.HexColor("#D7E2E3")
B=ParagraphStyle("B",fontName="DV",fontSize=9.2,leading=14,textColor=INK,spaceAfter=8)
S=ParagraphStyle("S",parent=B,fontSize=7.7,leading=11.3,textColor=MUTED)
H1=ParagraphStyle("H1",fontName="DVB",fontSize=24,leading=29,textColor=INK,spaceAfter=16)
H2=ParagraphStyle("H2",fontName="DVB",fontSize=12,leading=16,textColor=TEAL,spaceBefore=8,spaceAfter=7)
T=ParagraphStyle("T",fontName="DVB",fontSize=38,leading=43,textColor=INK,spaceAfter=18)
K=ParagraphStyle("K",fontName="DVB",fontSize=8.5,leading=12,textColor=TEAL,spaceAfter=11)
C=ParagraphStyle("C",fontName="DV",fontSize=8,leading=11.5,textColor=INK)
story=[]
def P(x,s=B): return Paragraph(x,s)
def section(n,x): story.extend([PageBreak(),P(f"{n:02d} / GLOBO RADIANTE",K),P(x,H1)])
def tab(headers,rows,widths):
    data=[[P(f"<b>{x}</b>",C) for x in headers]]+[[P(str(x),C) for x in r] for r in rows]
    t=Table(data,colWidths=widths,repeatRows=1,hAlign="LEFT")
    t.setStyle(TableStyle([("BACKGROUND",(0,0),(-1,0),PALE),("LINEBELOW",(0,0),(-1,0),.7,TEAL),("LINEBELOW",(0,1),(-1,-1),.35,LINE),("VALIGN",(0,0),(-1,-1),"TOP"),("PADDING",(0,0),(-1,-1),7)]))
    story.extend([t,Spacer(1,8)])

story += [P("DOCUMENTACIÓN TÉCNICA  /  VERSIÓN 1.0",K),Spacer(1,28),P("Globo<br/>Radiante",T),P("Nodo portátil para estimar la temperatura radiante media",ParagraphStyle("sub",parent=B,fontSize=15,leading=21,textColor=MUTED)),Spacer(1,8),P("Sistema basado en ESP32 que combina temperatura del aire, temperatura de globo y humedad relativa. Presenta las mediciones en OLED y transmite resultados a FIWARE mediante WiFi."),P("La electrónica se aloja en una carcasa impresa en PLA con tapa atornillada. Incorpora batería de litio de 1000 mAh, carga USB-C, UPS, switch y botón multifunción."),Spacer(1,8)]
tab(["Identificación","Descripción"],[("Dispositivo","Globo Radiante"),("Hostname","globo-mrt-01"),("Controlador","ESP32"),("Variables","Ta, Tg, humedad relativa y MRT"),("Código fuente",f'<link href="{REPO}" color="#287D78">{REPO}</link>'),("Revisión","1.0 - septiembre de 2026")],[145,330])
story += [Spacer(1,15),P("Alcance",H2),P("Describe el principio, componentes, construcción y operación. El código fuente se conserva en el repositorio enlazado. Calibración, autonomía y validación física deben comprobarse sobre el prototipo final.",S)]

section(2,"Principio de medición")
story += [P("La temperatura radiante media (MRT) representa el efecto térmico conjunto de las superficies circundantes. Puede diferir de la temperatura del aire por paredes, ventanas, equipos u otras superficies calientes o frías."),P("El DS18B20 mide la temperatura de globo Tg. El HDC1080 mide la temperatura del aire Ta y la humedad relativa fuera del globo. El firmware combina Ta y Tg para estimar MRT."),P("Variables",H2)]
tab(["Magnitud","Origen","Unidad"],[("Ta","HDC1080: aire","°C"),("Tg","DS18B20: globo","°C"),("HR","HDC1080: humedad","%"),("MRT","Cálculo con Ta y Tg","°C")],[70,335,70])
story += [P("Ecuación implementada",H2),P("Forma simplificada de ISO 7726:1998 para convección natural, globo negro estándar de 0,15 m y emisividad 0,95:"),Table([[P("<b>MRT = [(Tg + 273,15)<super>4</super> + 4 × 10<super>7</super> |Tg-Ta|<super>1/4</super> (Tg-Ta)]<super>1/4</super> - 273,15</b>",B)]],colWidths=[475],style=TableStyle([("BACKGROUND",(0,0),(-1,-1),PALE),("BOX",(0,0),(-1,-1),.5,LINE),("PADDING",(0,0),(-1,-1),11)])),Spacer(1,10),P("La humedad no interviene en la ecuación. La estimación requiere equilibrio térmico y convección natural. Con corrientes de aire se necesita medir velocidad del aire y evaluar convección forzada."),P("La carcasa de PLA protege la electrónica y no se considera automáticamente un globo normalizado. Deben verificarse diámetro, emisividad, acabado negro y montaje. Esta implementación referencia ISO 7726:1998 y no constituye certificación normativa.",S)]

section(3,"Construcción y alimentación")
story += [P("La carcasa se fabrica en PLA por impresión 3D y se cierra con una tapa atornillada. Permite acceso a pantalla, USB-C, switch y botón, y facilita mantenimiento e incorporación posterior de fotografías."),P("Cadena de energía",H2)]
tab(["Etapa","Función"],[("USB-C","Entrada de carga"),("Batería de litio","Reserva de 1000 mAh"),("UPS","Gestión de carga y salida de 5 V"),("Regulación","Adaptación de 5 V al dominio de 3,3 V"),("Electrónica","ESP32, OLED y sensores a 3,3 V")],[140,335])
story += [P("Un capacitor de 1000 µF / 12 V se conecta en paralelo entre 5 V y GND a la salida de la UPS. Aporta reserva local y filtrado. Los 12 V son su tensión nominal; el bus sigue siendo de 5 V."),P("El capacitor no regula tensión ni sustituye protecciones. Deben comprobarse polaridad, corriente de carga, regulación y comportamiento de la UPS. Los 1000 mAh no permiten afirmar autonomía sin medir consumo real.",S),P("Interconexión",H2)]
tab(["Elemento","Conexión"],[("OLED SSD1306","I2C: SDA GPIO21, SCL GPIO22; 0x3C"),("HDC1080","I2C compartido; 0x40; 3,3 V"),("DS18B20","DATA GPIO27; pull-up 4,7 kΩ; 3,3 V"),("Botón","GPIO0 a GND; INPUT_PULLUP"),("Switch","Corte general de alimentación")],[140,335])

section(4,"Interacción y firmware")
story += [P("La pantalla alterna cada 5 s entre valores crudos, MRT grande, tiempo para el próximo envío e IP de red."),P("Controles",H2)]
tab(["Acción","Respuesta"],[("Clic corto","Cambia de vista"),("Clic con pantalla apagada","Despierta sin avanzar"),("180 s de inactividad","Apaga la pantalla; medición y envío continúan"),("Pulsación de 4 s","Entra o sale del modo desarrollador"),("Switch","Enciende o apaga el dispositivo")],[160,315])
story += [P("Estados",H2)]
tab(["Estado","Responsabilidad"],[("INICIO","Carga configuración y conecta WiFi"),("LECTURA","Adquiere, valida, calcula y muestra"),("ENVIO","Construye NGSIv2 y transmite promedios"),("DESARROLLADOR","Configura red y sistema; permite OTA")],[115,360])
story += [P("El payload envía tempAire, tempGlobo, mrt y humedad como Float. La entidad Orion debe existir antes. Al salir del modo desarrollador se detienen DNS y servidor, se desactiva el AP y se regresa a modo estación."),P("La lectura inválida vacía acumuladores y suspende el envío. MRT se calcula por muestra y después se promedia. Se conservan StateMachine, WiFiManager, TokenManager, ButtonHandler y DevWebOTA."),P("Repositorio",H2),P(f'<link href="{REPO}" color="#287D78">{REPO}</link>'),P("Contiene firmware, instrucciones, pruebas, guía técnica y la fuente editable de este PDF.",S)]

section(5,"Puesta en servicio")
story += [P("1. Verificar polaridades y tensiones de 5 V y 3,3 V.<br/>2. Instalar bibliotecas y cargar main/main.ino.<br/>3. Configurar WiFi, FIWARE y autenticación desde el portal.<br/>4. Comprobar Ta, Tg y HR; esperar estabilidad térmica.<br/>5. Confirmar los cuatro atributos en Orion.<br/>6. Medir consumo, autonomía, carga y respuesta térmica."),P("La función MRT pasó 298 casos numéricos y diez archivos base se comprobaron con SHA-256. Quedan pendientes compilación completa en placa, calibración y pruebas físicas de sensores, alimentación, transmisión, reconexión y OTA."),P("Referencias",H2),P("[1] ISO 7726:1998, anexo B. https://standards.iteh.ai/catalog/standards/iso/99f92eea-d1b3-48b4-8a3c-8e5a5112718a/iso-7726-1998",S),P("[2] Texas Instruments, HDC1080. https://www.ti.com/lit/ds/symlink/hdc1080.pdf",S),P("[3] Analog Devices / Maxim, DS18B20. https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf",S),P("[4] Firmware base TARS. https://github.com/1cfer/Tars - commit 2439dd5a2378ba45309707a6d51de8fe334ec83c",S)]

def footer(canvas,doc):
    canvas.saveState(); w,_=A4; canvas.setStrokeColor(LINE); canvas.line(60,48,w-60,48)
    canvas.setFont("DV",7.3); canvas.setFillColor(MUTED); canvas.drawString(60,32,"GLOBO RADIANTE  /  DOCUMENTACIÓN TÉCNICA 1.0"); canvas.drawRightString(w-60,32,f"{doc.page:02d}"); canvas.restoreState()

OUT.parent.mkdir(parents=True,exist_ok=True)
SimpleDocTemplate(str(OUT),pagesize=A4,leftMargin=60,rightMargin=60,topMargin=55,bottomMargin=65,title="Globo Radiante - Documentación técnica",author="Proyecto Globo Radiante").build(story,onFirstPage=footer,onLaterPages=footer)
print(OUT)
