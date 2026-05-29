#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <limits>
#include <unordered_map>
#include <set>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <windows.h>

using namespace std;
using json = nlohmann::json;

const string EXT_HISTORIAL = ".hist";
const string EXT_CIFRADO   = ".enc";
const string EXT_LLAVE     = ".key";
const string EXT_PASSWORD  = ".pass";

// Retorna el numero de bytes del caracter UTF-8 que inicia en c, considerando caracteres especiales (especialmente la ñ)
int utf8CharLen(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    return 1;
}

// --- LOGICA DE ENCRIPTACION ---
unordered_map<string, string> encryptMap = {
    {"a", "U1"}, {"e", "U2"}, {"i", "U3"}, {"o", "U4"}, {"u", "U5"},
    {"b", "m1"}, {"c", "m2"}, {"d", "m3"}, {"f", "m4"}, {"g", "m5"},
    {"h", "m6"}, {"j", "m7"}, {"k", "m8"}, {"l", "m9"}, {"m", "m10"},
    {"n", "m11"}, {"ñ", "m12"}, {"p", "m13"}, {"q", "m14"}, {"r", "m15"},
    {"s", "m16"}, {"t", "m17"}, {"v", "m18"}, {"w", "m19"},
    {"x", "m20"}, {"y", "m21"}, {"z", "m22"},
    {"B", "g1"},  {"C", "g2"},  {"D", "g3"},  {"F", "g4"},  {"G", "g5"},
    {"H", "g6"},  {"J", "g7"},  {"K", "g8"},  {"L", "g9"},  {"M", "g10"},
    {"N", "g11"}, {"Ñ", "g12"}, {"P", "g13"}, {"Q", "g14"}, {"R", "g15"},
    {"S", "g16"}, {"T", "g17"}, {"V", "g18"}, {"W", "g19"},
    {"X", "g20"}, {"Y", "g21"}, {"Z", "g22"}
};

string encriptar(const string& texto) {
    string cifrado;
    for (size_t i = 0; i < texto.length(); ) {
        int len = utf8CharLen((unsigned char)texto[i]);
        string sub = texto.substr(i, len);
        if (encryptMap.count(sub)) {
            cifrado += "[" + encryptMap[sub] + "]";
        } else {
            cifrado += sub;
        }
        i += len;
    }
    return cifrado;
}

unordered_map<string, string> decryptMap = []() {
    unordered_map<string, string> m;
    for (auto& p : encryptMap)
        m[p.second] = p.first;
    return m;
}();

string desencriptar(const string& cifrado) {
    string original;
    size_t i = 0;
    while (i < cifrado.size()) {
        if (cifrado[i] == '[') {
            size_t cierre = cifrado.find(']', i + 1);
            if (cierre != string::npos) {
                string codigo = cifrado.substr(i + 1, cierre - i - 1);
                auto it = decryptMap.find(codigo);
                if (it != decryptMap.end()) {
                    original += it->second;
                    i = cierre + 1;
                    continue;
                }
            }
        }
        original += cifrado[i];
        i++;
    }
    return original;
}

void crearDirectorioRecursivo(const string& ruta) {
    for (size_t i = 1; i <= ruta.size(); i++) {
        if (i == ruta.size() || ruta[i] == '/' || ruta[i] == '\\')
            CreateDirectoryA(ruta.substr(0, i).c_str(), nullptr);
    }
}

const string MI_API_KEY = "AIzaSyBOfIq453ZzyFIDJXbFOPmP4CUthlGfGDs";

string aMinusculas(string cadena) {
    for (size_t i = 0; i < cadena.length(); ) {
        unsigned char c = (unsigned char)cadena[i];
        if (c < 0x80) {
            cadena[i] = (char)tolower(c);
            i++;
        } else {
            if (c == 0xC3 && i + 1 < cadena.length() && (unsigned char)cadena[i+1] == 0x91)
                cadena[i+1] = (char)0xB1;
            i += utf8CharLen(c);
        }
    }
    return cadena;
}

void reproducirAudio(const string &texto)
{
    string comando = 
    "powershell -NoProfile -NonInteractive -WindowStyle Hidden "
   " -Command \"Add-Type -AssemblyName System.Speech; "
    "(New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('" + texto + "')\"";

    STARTUPINFOA infoInicio = {};
    infoInicio.cb = sizeof(infoInicio);

    infoInicio.dwFlags = STARTF_USESHOWWINDOW; 
    infoInicio.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION infoProceso = {};

    if (CreateProcessA(nullptr, const_cast<char *>(comando.c_str()), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &infoInicio, &infoProceso))
    {
        WaitForSingleObject(infoProceso.hProcess, INFINITE);

        CloseHandle(infoProceso.hProcess);
        CloseHandle(infoProceso.hThread);
    }
}

struct NodoAVL
{
    string palabra;
    string traduccion;
    string idioma;
    int contadorBusqueda;
    int altura;
    NodoAVL *izquierdo;
    NodoAVL *derecho;

    NodoAVL(const string &pal, const string &trad, const string &id, int cont)
        : palabra(pal), traduccion(trad), idioma(id), contadorBusqueda(cont),
          altura(1), izquierdo(nullptr), derecho(nullptr) {}
};

class ArbolAVL
{
private:
    NodoAVL *raiz;

    int altura(NodoAVL *n)
    {
        return n ? n->altura : 0;
    }

    int factorBalance(NodoAVL *n)
    {
        return n ? altura(n->izquierdo) - altura(n->derecho) : 0;
    }

    void actualizarAltura(NodoAVL *n)
    {
        if (n)
            n->altura = 1 + max(altura(n->izquierdo), altura(n->derecho));
    }

    NodoAVL *rotarDerecha(NodoAVL *y)
    {
        NodoAVL *x = y->izquierdo;
        NodoAVL *T2 = x->derecho;
        x->derecho = y;
        y->izquierdo = T2;
        actualizarAltura(y);
        actualizarAltura(x);
        return x;
    }

    NodoAVL *rotarIzquierda(NodoAVL *x)
    {
        NodoAVL *y = x->derecho;
        NodoAVL *T2 = y->izquierdo;
        y->izquierdo = x;
        x->derecho = T2;
        actualizarAltura(x);
        actualizarAltura(y);
        return y;
    }

    NodoAVL *balancear(NodoAVL *n)
    {
        actualizarAltura(n);
        int fb = factorBalance(n);

        if (fb > 1 && factorBalance(n->izquierdo) >= 0)
            return rotarDerecha(n);

        if (fb > 1 && factorBalance(n->izquierdo) < 0)
        {
            n->izquierdo = rotarIzquierda(n->izquierdo);
            return rotarDerecha(n);
        }

        if (fb < -1 && factorBalance(n->derecho) <= 0)
            return rotarIzquierda(n);

        if (fb < -1 && factorBalance(n->derecho) > 0)
        {
            n->derecho = rotarDerecha(n->derecho);
            return rotarIzquierda(n);
        }

        return n;
    }

    NodoAVL *insertar(NodoAVL *n, const string &pal, const string &trad,
                      const string &id, int cont)
    {
        if (!n)
            return new NodoAVL(pal, trad, id, cont);

        if (pal < n->palabra)
            n->izquierdo = insertar(n->izquierdo, pal, trad, id, cont);
        else if (pal > n->palabra)
            n->derecho = insertar(n->derecho, pal, trad, id, cont);
        else
        {
            n->traduccion = trad;
            n->idioma = id;
            n->contadorBusqueda = cont;
            return n;
        }

        return balancear(n);
    }

    NodoAVL *minimoNodo(NodoAVL *n)
    {
        while (n->izquierdo)
            n = n->izquierdo;
        return n;
    }

    NodoAVL *eliminar(NodoAVL *n, const string &pal)
    {
        if (!n)
            return nullptr;

        if (pal < n->palabra)
            n->izquierdo = eliminar(n->izquierdo, pal);
        else if (pal > n->palabra)
            n->derecho = eliminar(n->derecho, pal);
        else
        {
            if (!n->izquierdo || !n->derecho)
            {
                NodoAVL *temp = n->izquierdo ? n->izquierdo : n->derecho;
                delete n;
                return temp;
            }
            NodoAVL *suc = minimoNodo(n->derecho);
            n->palabra = suc->palabra;
            n->traduccion = suc->traduccion;
            n->idioma = suc->idioma;
            n->contadorBusqueda = suc->contadorBusqueda;
            n->derecho = eliminar(n->derecho, suc->palabra);
        }

        return balancear(n);
    }

    NodoAVL *buscar(NodoAVL *n, const string &pal)
    {
        if (!n || n->palabra == pal)
            return n;
        if (pal < n->palabra)
            return buscar(n->izquierdo, pal);
        return buscar(n->derecho, pal);
    }

    void inorden(NodoAVL *n, vector<NodoAVL *> &resultado)
    {
        if (!n)
            return;
        inorden(n->izquierdo, resultado);
        resultado.push_back(n);
        inorden(n->derecho, resultado);
    }

    void destruir(NodoAVL *n)
    {
        if (!n)
            return;
        destruir(n->izquierdo);
        destruir(n->derecho);
        delete n;
    }

public:
    ArbolAVL() : raiz(nullptr) {}
    ~ArbolAVL() { destruir(raiz); }

    void insertar(const string &pal, const string &trad = "", const string &id = "", int cont = 1)
    {
        raiz = insertar(raiz, aMinusculas(pal), trad, id, cont); // Se inserta en minusculas
    }

    bool eliminar(const string &pal)
    {
        string palMin = aMinusculas(pal);
        if (!buscar(raiz, palMin))
            return false;
        raiz = eliminar(raiz, palMin);
        return true;
    }

    NodoAVL *buscar(const string &pal)
    {
        return buscar(raiz, aMinusculas(pal)); 
    }

    void incrementarContador(const string &pal)
    {
        NodoAVL *n = buscar(raiz, aMinusculas(pal));
        if (n)
            n->contadorBusqueda++;
    }

    vector<NodoAVL *> obtenerTodos()
    {
        vector<NodoAVL *> resultado;
        inorden(raiz, resultado);
        return resultado;
    }

    bool estaVacio() { return raiz == nullptr; }

    void cargarDesdeArchivo(const string &ruta)
    {
        ifstream archivo(ruta);
        if (!archivo.is_open())
            return;
        string linea;
        while (getline(archivo, linea))
        {
            if (linea.empty())
                continue;
            size_t p1 = linea.find('|');
            size_t p2 = linea.find('|', p1 + 1);
            size_t p3 = linea.find('|', p2 + 1);
            if (p1 == string::npos || p2 == string::npos || p3 == string::npos)
                continue;
            string pal = linea.substr(0, p1);
            string trad = linea.substr(p1 + 1, p2 - p1 - 1);
            string id = linea.substr(p2 + 1, p3 - p2 - 1);
            int cont = stoi(linea.substr(p3 + 1));
            insertar(pal, trad, id, cont);
        }
        archivo.close();
    }

    void guardarEnArchivo(const string &ruta)
    {
        ofstream archivo(ruta);
        for (NodoAVL *n : obtenerTodos())
        {
            archivo << n->palabra << "|"
                    << n->traduccion << "|"
                    << n->idioma << "|"
                    << n->contadorBusqueda << "\n";
        }
        archivo.close();
    }

    void cargarDesdeArchivoEncriptado(const string &ruta)
    {
        ifstream archivo(ruta);
        if (!archivo.is_open())
            return;
        string linea;
        while (getline(archivo, linea))
        {
            if (linea.empty())
                continue;
            string lineaDec = desencriptar(linea);
            size_t p1 = lineaDec.find('|');
            size_t p2 = lineaDec.find('|', p1 + 1);
            size_t p3 = lineaDec.find('|', p2 + 1);
            if (p1 == string::npos || p2 == string::npos || p3 == string::npos)
                continue;
            string pal  = lineaDec.substr(0, p1);
            string trad = lineaDec.substr(p1 + 1, p2 - p1 - 1);
            string id   = lineaDec.substr(p2 + 1, p3 - p2 - 1);
            int cont    = stoi(lineaDec.substr(p3 + 1));
            insertar(pal, trad, id, cont);
        }
        archivo.close();
    }
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

string traducir(const string &texto, const string &lenguajeDestino)
{
    CURL *curl;
    CURLcode res;
    string respuestaJson;

    curl = curl_easy_init();
    if (curl)
    {
        char *textoEscapado = curl_easy_escape(curl, texto.c_str(), texto.length());
        string url = "https://translation.googleapis.com/language/translate/v2?q=" +
                     string(textoEscapado) + "&target=" + lenguajeDestino + "&key=" + MI_API_KEY;
        curl_free(textoEscapado);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respuestaJson);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            auto j = json::parse(respuestaJson);
            if (j.contains("data"))
            {
                return j["data"]["translations"][0]["translatedText"];
            }
        }
    }
    return "[Error de conexion o API Key invalida]";
}

// Anchos de separadores consistentes: 2 espacios de margen + 60 caracteres = 62 en total
#define SEP_M "  ============================================================"
#define SEP_S "  ------------------------------------------------------------"

void mostrarSugerencias(ArbolAVL &arbol, const string &usuario)
{
    auto todos = arbol.obtenerTodos();
    if (todos.empty())
        return;

    cout << "  SUGERENCIAS PARA " << usuario << " (MAS BUSCADAS):" << endl;
    int limite = min((int)todos.size(), 3);
    for (int i = 0; i < limite; i++) {
        cout << "    [" << (i + 1) << "] "
             << left << setw(13) << todos[i]->palabra
             << " -> "
             << setw(16) << todos[i]->traduccion
             << " (" << todos[i]->contadorBusqueda << " busquedas)" << endl;
    }
    cout << SEP_S << endl;
}

void mostrarHistorial(ArbolAVL &arbol)
{
    auto todos = arbol.obtenerTodos();
    if (todos.empty()) {
        cout << endl << "  [!] Sin historial de busquedas." << endl;
        return;
    }

    cout << endl;
    cout << SEP_M << endl;
    cout << "              HISTORIAL COMPLETO DE BUSQUEDAS" << endl;
    cout << SEP_M << endl;
    cout << "  " << left
         << setw(18) << "PALABRA"
         << setw(22) << "TRADUCCION"
         << setw(8)  << "IDIOMA"
         << "BUSQUEDAS" << endl;
    cout << SEP_S << endl;
    for (NodoAVL *n : todos) {
        cout << "  " << left
             << setw(18) << n->palabra
             << setw(22) << n->traduccion
             << setw(8)  << n->idioma
             << n->contadorBusqueda << endl;
    }
    cout << SEP_M << endl;
}

void guardarLlave(const string &dir)
{
    ofstream fLlave(dir + "/llave" + EXT_LLAVE);
    fLlave << "UMG\n";
    fLlave.close();
}

bool validarLlave(const string &dir)
{
    ifstream fLlave(dir + "/llave" + EXT_LLAVE);
    if (!fLlave.is_open())
        return false;
    string contenido;
    getline(fLlave, contenido);
    fLlave.close();
    return contenido == "UMG";
}

void guardarHistorialEncriptado(const string &usuario, ArbolAVL &arbol)
{
    string dir = "usuarios/" + usuario;
    crearDirectorioRecursivo(dir);
    ofstream fOrig(dir + "/historial_original" + EXT_HISTORIAL), fCif(dir + "/historial_cifrado" + EXT_CIFRADO);
    for (NodoAVL *n : arbol.obtenerTodos()) {
        string linea = n->palabra + "|" + n->traduccion + "|" + n->idioma + "|" + to_string(n->contadorBusqueda);
        fOrig << linea << "\n"; fCif << encriptar(linea) << "\n";
    }

    fOrig.close();
    fCif.close();
    guardarLlave(dir);
}

void limpiarPantalla()
{
    cout << "\033[2J\033[1;1H";
}

void pausar() {
    cout << endl << "  Presione ENTER para continuar..." << flush;
    cin.get();
}

string gestionarUsuario()
{
    string usuario;
    string password;
    int opcion;
    while (true) {
        limpiarPantalla();
        cout << SEP_M << endl;
        cout << "                  BIENVENIDO AL TRADUCTOR UMG" << endl;
        cout << SEP_M << endl;
        cout << "  [1] Iniciar sesion" << endl;
        cout << "  [2] Registrar nuevo usuario" << endl;
        cout << "  [3] Salir del programa" << endl;
        cout << SEP_S << endl;
        cout << "  Seleccione una opcion: ";
        cin >> opcion;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (opcion)
        {
        case 1:
            cout << endl << "  --- INICIO DE SESION ---" << endl;
            cout << "  ID de Usuario : "; cin >> usuario;
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            {
                string rutaPass = "usuarios/" + usuario + "/pass" + EXT_PASSWORD;
                ifstream fPass(rutaPass);
                if (fPass.good()) {
                    cout << "  Contrasena    : "; cin >> password;
                    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    string passAlmacenada; getline(fPass, passAlmacenada); fPass.close();
                    if (encriptar(password) == passAlmacenada) {
                        cout << endl << "  [OK] Bienvenido de nuevo, " << usuario << "!" << endl;
                        pausar(); return usuario;
                    } else { cout << endl << "  [X] Contrasena incorrecta." << endl; pausar(); }
                } else { cout << endl << "  [!] El usuario no existe." << endl; pausar(); }
            }
            break;
        case 2:
            cout << endl << "  --- REGISTRO DE USUARIO ---" << endl;
            cout << "  Nuevo usuario : "; cin >> usuario;
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            {
                string dir = "usuarios/" + usuario;
                ifstream verificar(dir + "/pass" + EXT_PASSWORD);
                if (verificar.good()) { cout << endl << "  [!] Este usuario ya existe." << endl; pausar(); }
                else {
                    cout << "  Nueva contrasena: "; cin >> password;
                    cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    crearDirectorioRecursivo(dir);
                    ofstream f(dir + "/pass" + EXT_PASSWORD); f << encriptar(password); f.close();
                    ofstream(dir + "/historial_cifrado" + EXT_CIFRADO).close();
                    guardarLlave(dir);
                    cout << endl << "  [OK] Usuario '" << usuario << "' registrado con exito!" << endl;
                    pausar();
                    return usuario;
                }
            }
            break;
        case 3: exit(0);
        default: cout << "  [!] Opcion invalida." << endl; pausar(); break;
        }
    }
}

int main()
{
    string usuario = gestionarUsuario();
    string dirUsuario = "usuarios/" + usuario;

    if (!validarLlave(dirUsuario)) {
        cout << endl << "  [ERROR] La llave de cifrado ha sido alterada o es invalida." << endl;
        cout << "  [Acceso bloqueado. Contacte al administrador.]" << endl;
        pausar(); return 1;
    }

    string rutaCifrado = dirUsuario + "/historial_cifrado" + EXT_CIFRADO;
    ArbolAVL historial;
    historial.cargarDesdeArchivoEncriptado(rutaCifrado);

    int opcion;
    do {
        limpiarPantalla();
        cout << SEP_M << endl;
        cout << "  " << left << setw(30) << "TRADUCTOR UMG" << "Usuario: " << usuario << endl;
        cout << SEP_M << endl;
        mostrarSugerencias(historial, usuario);
        cout << "  [1] Traducir palabra o frase" << endl;
        cout << "  [2] Agregar traduccion manualmente" << endl;
        cout << "  [3] Eliminar del historial" << endl;
        cout << "  [4] Ver historial completo" << endl;
        cout << "  [5] Salir y guardar sesion" << endl;
        cout << SEP_S << endl;
        cout << "  Opcion: ";
        cin >> opcion;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (opcion) {
        case 1: {
            string pal, idio, res;
            cout << endl << "  >>> TRADUCIR TEXTO <<<" << endl;
            cout << "  Texto              : "; getline(cin, pal);
            cout << "  Idioma (en/fr/it/de/es): "; cin >> idio;
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');

            NodoAVL *cache = historial.buscar(pal);
            if (cache && cache->idioma == idio) {
                res = cache->traduccion;
                cout << endl << "  [CACHE] Traduccion : " << res << endl;
                historial.incrementarContador(pal);
            } else {
                res = traducir(pal, idio);
                cout << endl << "  [API]   Traduccion : " << res << endl;
                int cont = cache ? cache->contadorBusqueda + 1 : 1;
                if (cache) historial.eliminar(pal);
                historial.insertar(pal, res, idio, cont);
            }
            cout << "  [INFO]  Reproduciendo audio..." << endl;
            reproducirAudio(res); 
            pausar();
            break;
        }
        case 2: {
            string pal, trad, idio;
            cout << endl << "  >>> AGREGAR MANUALMENTE <<<" << endl;
            cout << "  Original   : "; getline(cin, pal);
            cout << "  Traduccion : "; getline(cin, trad);
            cout << "  Idioma     : "; cin >> idio;
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
            historial.insertar(pal, trad, idio, 1);
            cout << endl << "  [OK] Guardado en historial." << endl; pausar();
            break;
        }
        case 3: {
            string pal;
            cout << endl << "  >>> ELIMINAR DEL HISTORIAL <<<" << endl;
            cout << "  Texto a borrar : "; getline(cin, pal);
            if (historial.eliminar(pal)) cout << "  [OK] Registro eliminado." << endl;
            else cout << "  [!] No se encontro el texto." << endl;
            pausar(); break;
        }
        case 4:
            mostrarHistorial(historial);
            pausar();
            break;
        case 5:
        {
            guardarHistorialEncriptado(usuario, historial);
            cout << endl << "  [OK] Sesion guardada. Hasta pronto, " << usuario << "!" << endl;
            break;
        }
        default:
            cout << "  [!] Opcion no valida." << endl;
            pausar();
            break;
        }

    } while (opcion != 5);

    return 0;
}