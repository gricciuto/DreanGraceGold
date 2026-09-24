#include <DIYables_LCD_I2C.h>
#include <EEPROM.h>
DIYables_LCD_I2C pantalla(0x27, 16, 2);

//Parametros de configuracion persistentes
const uint16_t n_magic = 0xFAFA;
struct Configuracion {
  uint16_t rpm_centrifugado;
  uint16_t rpm_lavado;
  uint8_t tiempo_lavado;
  uint8_t tiempo_centrifugado;
  bool sensor_pos_enabled; // Este va a ser en caso de que pueda hacer andar el sensor magnetico del tambor que sirve para que la tapa quede mirando para arriba
  uint16_t magic; //Esto dice GPT que lo use para saber si la configuracion almacenada en la EEPROM es correcta o es la inicial.
};


Configuracion generarConfiguracionInicial(){ //En caso que sea la primera vez que se inicia o no se haya modificado la EEPROM
  Configuracion config_inicial;
  config_inicial.magic = n_magic;
  config_inicial.rpm_centrifugado = 10000;
  config_inicial.rpm_lavado = 300;
  config_inicial.tiempo_centrifugado = 10;
  config_inicial.tiempo_lavado = 50;
  config_inicial.sensor_pos_enabled = true;
  return config_inicial;
}

Configuracion config;
void cargarConfiguracion(){
  EEPROM.get(0,config);
  if (config.magic != n_magic){
    //La config que hay en la EEPROM es incorrecta
    config = generarConfiguracionInicial();
  }
}



// Config de botones

const byte BTN_ACEPTAR   = 4;
const byte BTN_DIRECCION = 5;
const byte BTN_CANCELAR  = 2;

bool anteriorAceptar   = HIGH;
bool anteriorDireccion = HIGH;
bool anteriorCancelar  = HIGH;

bool botonPresionado(byte pin, bool &estadoAnterior) {

  bool estadoActual = digitalRead(pin);

  bool presionado =
    (estadoAnterior == HIGH && estadoActual == LOW);

  estadoAnterior = estadoActual;

  return presionado;
}


// =====================================================
//                 ESTRUCTURA DEL MENÚ
// =====================================================

struct OpcionMenu {

  const char* nombre;

  // Función que se ejecuta al presionar ACEPTAR
  void (*accion)();
};


// =====================================================
//              VARIABLES DEL SISTEMA
// =====================================================

byte opcionActual = 0;


// =====================================================
//                 ESTADOS DEL MENÚ
// =====================================================

enum MenuActual {

  MENU_PRINCIPAL,

  MENU_LAVADO,

  MENU_CENTRIFUGADO,

  MENU_CONFIGURACION
};

MenuActual menuActual = MENU_PRINCIPAL;


// =====================================================
//             DECLARACIÓN DE FUNCIONES
// =====================================================

// Menús
void mostrarMenuPrincipal();
void mostrarMenuLavado();
void mostrarMenuCentrifugado();
void mostrarMenuConfiguracion();

// Acciones
void abrirLavado();
void abrirCentrifugado();
void abrirConfiguracion();

void iniciarLavado();
void configurarTemperatura();
void configurarTiempoLavado();

void iniciarCentrifugado();
void configurarRPM();
void configurarTiempoCentrifugado();

void calibrarSensor();
void mostrarInformacion();

void volverMenuAnterior();


// =====================================================
//                  MENÚ PRINCIPAL
// =====================================================

OpcionMenu menuPrincipal[] = {

  {"Lavado", abrirLavado},

  {"Centrifugado", abrirCentrifugado},

  {"Configuracion", abrirConfiguracion}

};

const byte cantidadMenuPrincipal =
  sizeof(menuPrincipal) / sizeof(menuPrincipal[0]);


// =====================================================
//                    MENÚ LAVADO
// =====================================================

OpcionMenu menuLavado[] = {

  {"Iniciar lavado", iniciarLavado},

  {"Temperatura", configurarTemperatura},

  {"Tiempo", configurarTiempoLavado}

};

const byte cantidadMenuLavado =
  sizeof(menuLavado) / sizeof(menuLavado[0]);


// =====================================================
//                 MENÚ CENTRIFUGADO
// =====================================================

OpcionMenu menuCentrifugado[] = {

  {"Iniciar", iniciarCentrifugado},

  {"RPM", configurarRPM},

  {"Tiempo", configurarTiempoCentrifugado}

};

const byte cantidadMenuCentrifugado =
  sizeof(menuCentrifugado) / sizeof(menuCentrifugado[0]);


// =====================================================
//                MENÚ CONFIGURACIÓN
// =====================================================

OpcionMenu menuConfiguracion[] = {

  {"Calibrar sensor", calibrarSensor},

  {"Informacion", mostrarInformacion}

};

const byte cantidadMenuConfiguracion =
  sizeof(menuConfiguracion) / sizeof(menuConfiguracion[0]);


// =====================================================
//                MENÚ ANTERIOR
// =====================================================

// Guardamos de dónde venimos para que CANCELAR
// pueda volver al menú anterior.

MenuActual menuAnterior = MENU_PRINCIPAL;


// =====================================================
//                     SETUP
// =====================================================

void setup() {

  pantalla.init();
  pantalla.backlight();

  pinMode(BTN_ACEPTAR, INPUT_PULLUP);
  pinMode(BTN_DIRECCION, INPUT_PULLUP);
  pinMode(BTN_CANCELAR, INPUT_PULLUP);

  cargarConfiguracion();

  mostrarMenu();
}


// =====================================================
//                      LOOP
// =====================================================

void loop() {

  // ---------------------------------------------------
  // BOTÓN DIRECCIÓN
  // ---------------------------------------------------

  if (botonPresionado(BTN_DIRECCION, anteriorDireccion)) {

    opcionActual++;

    byte cantidad = cantidadOpcionesMenuActual();

    if (opcionActual >= cantidad) {

      opcionActual = 0;
    }

    mostrarMenu();
  }


  // ---------------------------------------------------
  // BOTÓN ACEPTAR
  // ---------------------------------------------------

  if (botonPresionado(BTN_ACEPTAR, anteriorAceptar)) {

    ejecutarOpcionActual();
  }


  // ---------------------------------------------------
  // BOTÓN CANCELAR
  // ---------------------------------------------------

  if (botonPresionado(BTN_CANCELAR, anteriorCancelar)) {

    cancelar();
  }
}


// =====================================================
//          CANTIDAD DE OPCIONES DEL MENÚ ACTUAL
// =====================================================

byte cantidadOpcionesMenuActual() {

  switch (menuActual) {

    case MENU_PRINCIPAL:
      return cantidadMenuPrincipal;

    case MENU_LAVADO:
      return cantidadMenuLavado;

    case MENU_CENTRIFUGADO:
      return cantidadMenuCentrifugado;

    case MENU_CONFIGURACION:
      return cantidadMenuConfiguracion;
  }

  return 0;
}


// =====================================================
//                 MOSTRAR MENÚ
// =====================================================

void mostrarMenu() {

  pantalla.clear();

  const char* nombreActual;
  const char* nombreSiguiente;

  byte cantidad = cantidadOpcionesMenuActual();

  byte siguiente = opcionActual + 1;

  if (siguiente >= cantidad) {

    siguiente = 0;
  }


  // ---------------------------------------------------
  // OBTENER NOMBRE DE LA OPCIÓN ACTUAL
  // ---------------------------------------------------

  nombreActual = obtenerNombreOpcion(opcionActual);

  nombreSiguiente = obtenerNombreOpcion(siguiente);


  // ---------------------------------------------------
  // MOSTRAR
  // ---------------------------------------------------

  pantalla.setCursor(0, 0);

  pantalla.print("> ");
  pantalla.print(nombreActual);


  pantalla.setCursor(0, 1);

  pantalla.print("  ");
  pantalla.print(nombreSiguiente);
}


// =====================================================
//             OBTENER NOMBRE DE UNA OPCIÓN
// =====================================================

const char* obtenerNombreOpcion(byte indice) {

  switch (menuActual) {

    case MENU_PRINCIPAL:
      return menuPrincipal[indice].nombre;

    case MENU_LAVADO:
      return menuLavado[indice].nombre;

    case MENU_CENTRIFUGADO:
      return menuCentrifugado[indice].nombre;

    case MENU_CONFIGURACION:
      return menuConfiguracion[indice].nombre;
  }

  return "";
}


// =====================================================
//              EJECUTAR OPCIÓN ACTUAL
// =====================================================

void ejecutarOpcionActual() {

  switch (menuActual) {

    case MENU_PRINCIPAL:

      menuPrincipal[opcionActual].accion();

      break;


    case MENU_LAVADO:

      menuLavado[opcionActual].accion();

      break;


    case MENU_CENTRIFUGADO:

      menuCentrifugado[opcionActual].accion();

      break;


    case MENU_CONFIGURACION:

      menuConfiguracion[opcionActual].accion();

      break;
  }
}


// =====================================================
//                   CANCELAR
// =====================================================

void cancelar() {

  if (menuActual == MENU_PRINCIPAL) {

    // Ya estamos en el menú principal.
    return;
  }

  menuActual = menuAnterior;

  opcionActual = 0;

  mostrarMenu();
}


// =====================================================
//            ABRIR SUBMENÚ DE LAVADO
// =====================================================

void abrirLavado() {

  menuAnterior = MENU_PRINCIPAL;

  menuActual = MENU_LAVADO;

  opcionActual = 0;

  mostrarMenu();
}


// =====================================================
//        ABRIR SUBMENÚ DE CENTRIFUGADO
// =====================================================

void abrirCentrifugado() {

  menuAnterior = MENU_PRINCIPAL;

  menuActual = MENU_CENTRIFUGADO;

  opcionActual = 0;

  mostrarMenu();
}


// =====================================================
//          ABRIR SUBMENÚ CONFIGURACIÓN
// =====================================================

void abrirConfiguracion() {

  menuAnterior = MENU_PRINCIPAL;

  menuActual = MENU_CONFIGURACION;

  opcionActual = 0;

  mostrarMenu();
}


// =====================================================
//                   ACCIONES DE LAVADO
// =====================================================

void iniciarLavado() {

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("Lavado iniciado");

  delay(1500);

  // Acá posteriormente pondrías:

  // encender bomba
  // controlar motor
  // controlar temperatura
  // leer tacómetro
  // etc.

  mostrarMenu();
}


void configurarTemperatura() {

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("Temperatura");

  pantalla.setCursor(0, 1);
  pantalla.print("40 C");

  delay(1500);

  mostrarMenu();
}


void configurarTiempoLavado() {

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("Tiempo lavado");

  pantalla.setCursor(0, 1);
  pantalla.print("30 min");

  delay(1500);

  mostrarMenu();
}


// =====================================================
//                ACCIONES CENTRIFUGADO
// =====================================================

void iniciarCentrifugado() {

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("Centrifugando");

  pantalla.setCursor(0, 1);
  pantalla.print("1200 RPM");

  delay(1500);

  // Acá iría posteriormente
  // el control real del motor.

  mostrarMenu();
}


void configurarRPM() {

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("RPM");

  pantalla.setCursor(0, 1);
  pantalla.print("1200");

  delay(1500);

  mostrarMenu();
}


void configurarTiempoCentrifugado() {

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("Tiempo");

  pantalla.setCursor(0, 1);
  pantalla.print("10 min");

  delay(1500);

  mostrarMenu();
}


// =====================================================
//                 CONFIGURACIÓN
// =====================================================

void calibrarSensor() {

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("Calibrando...");

  delay(2000);

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("Calibracion OK");

  delay(1500);

  mostrarMenu();
}


void mostrarInformacion() {

  pantalla.clear();

  pantalla.setCursor(0, 0);
  pantalla.print("Lavadora v1.0");

  pantalla.setCursor(0, 1);
  pantalla.print("Arduino");

  delay(2000);

  mostrarMenu();
}

