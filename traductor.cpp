#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <unordered_map>
#include <set>
#include <ctime>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <windows.h>

using namespace std;
using json = nlohmann::json;

// Diccionario actualizado para soportar caracteres multi-byte (ñ/Ñ) y reglas G/M de la Fase 2
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

// Función para encriptar manejando caracteres de 1 y 2 bytes (como la ñ)
string encriptar(const string& texto) {
    string cifrado;
    for (size_t i = 0; i < texto.length(); ) {
        bool encontrado = false;
        // Intento buscar coincidencia de 2 bytes (para caracteres especiales UTF-8)
        if (i + 1 < texto.length()) {
            string sub = texto.substr(i, 2);
            if (encryptMap.count(sub)) {
                cifrado += "[" + encryptMap[sub] + "]";
                i += 2;
                encontrado = true;
            }
        }
        // Si no fue de 2 bytes, intento de 1 byte
        if (!encontrado) {
            string sub = texto.substr(i, 1);
            if (encryptMap.count(sub)) {
                cifrado += "[" + encryptMap[sub] + "]";
            } else {
                cifrado += texto[i];
            }
            i++;
        }
    }
    return cifrado;
}

unordered_map<string, string> decryptMap = []() {
    unordered_map<string, string> m;
    for (auto& p : encryptMap) m[p.second] = p.first;
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
                if (decryptMap.count(codigo)) {
                    original += decryptMap[codigo];
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

// Mantenemos la función por utilidad, pero ya no se usa en el árbol para respetar el diseño lógico (Mayúsculas)
string aMinusculas(string cadena) {
    for (int i = 0; i < (int)cadena.length(); i++) cadena[i] = tolower(cadena[i]);
    return cadena;
}

void reproducirAudio(const string &texto) {
    string comando = "powershell -NoProfile -NonInteractive -WindowStyle Hidden -Command \"Add-Type -AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('" + texto + "')\"";
    STARTUPINFOA infoInicio = {};
    infoInicio.cb = sizeof(infoInicio);
    infoInicio.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    infoInicio.wShowWindow = SW_HIDE;
    infoInicio.hStdOutput = INVALID_HANDLE_VALUE;
    infoInicio.hStdError  = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION infoProceso = {};
    if (CreateProcessA(nullptr, const_cast<char *>(comando.c_str()), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &infoInicio, &infoProceso)) {
        WaitForSingleObject(infoProceso.hProcess, INFINITE);
        CloseHandle(infoProceso.hProcess);
        CloseHandle(infoProceso.hThread);
    }
}

struct NodoAVL {
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

class ArbolAVL {
private:
    NodoAVL *raiz;

    int altura(NodoAVL *n) { return n ? n->altura : 0; }
    int factorBalance(NodoAVL *n) { return n ? altura(n->izquierdo) - altura(n->derecho) : 0; }
    void actualizarAltura(NodoAVL *n) { if (n) n->altura = 1 + max(altura(n->izquierdo), altura(n->derecho)); }

    NodoAVL *rotarDerecha(NodoAVL *y) {
        NodoAVL *x = y->izquierdo;
        NodoAVL *T2 = x->derecho;
        x->derecho = y;
        y->izquierdo = T2;
        actualizarAltura(y); actualizarAltura(x);
        return x;
    }

    NodoAVL *rotarIzquierda(NodoAVL *x) {
        NodoAVL *y = x->derecho;
        NodoAVL *T2 = y->izquierdo;
        y->izquierdo = x;
        x->derecho = T2;
        actualizarAltura(x); actualizarAltura(y);
        return y;
    }

    NodoAVL *balancear(NodoAVL *n) {
        actualizarAltura(n);
        int fb = factorBalance(n);
        if (fb > 1 && factorBalance(n->izquierdo) >= 0) return rotarDerecha(n);
        if (fb > 1 && factorBalance(n->izquierdo) < 0) {
            n->izquierdo = rotarIzquierda(n->izquierdo);
            return rotarDerecha(n);
        }
        if (fb < -1 && factorBalance(n->derecho) <= 0) return rotarIzquierda(n);
        if (fb < -1 && factorBalance(n->derecho) > 0) {
            n->derecho = rotarDerecha(n->derecho);
            return rotarIzquierda(n);
        }
        return n;
    }

    NodoAVL *insertar(NodoAVL *n, const string &pal, const string &trad, const string &id, int cont) {
        if (!n) return new NodoAVL(pal, trad, id, cont);
        if (pal < n->palabra) n->izquierdo = insertar(n->izquierdo, pal, trad, id, cont);
        else if (pal > n->palabra) n->derecho = insertar(n->derecho, pal, trad, id, cont);
        else {
            n->traduccion = trad; n->idioma = id; n->contadorBusqueda = cont;
            return n;
        }
        return balancear(n);
    }

    NodoAVL *minimoNodo(NodoAVL *n) { while (n->izquierdo) n = n->izquierdo; return n; }

    NodoAVL *eliminar(NodoAVL *n, const string &pal) {
        if (!n) return nullptr;
        if (pal < n->palabra) n->izquierdo = eliminar(n->izquierdo, pal);
        else if (pal > n->palabra) n->derecho = eliminar(n->derecho, pal);
        else {
            if (!n->izquierdo || !n->derecho) {
                NodoAVL *temp = n->izquierdo ? n->izquierdo : n->derecho;
                delete n; return temp;
            }
            NodoAVL *suc = minimoNodo(n->derecho);
            n->palabra = suc->palabra; n->traduccion = suc->traduccion;
            n->idioma = suc->idioma; n->contadorBusqueda = suc->contadorBusqueda;
            n->derecho = eliminar(n->derecho, suc->palabra);
        }
        return balancear(n);
    }

    NodoAVL *buscar(NodoAVL *n, const string &pal) {
        if (!n || n->palabra == pal) return n;
        if (pal < n->palabra) return buscar(n->izquierdo, pal);
        return buscar(n->derecho, pal);
    }

    void inorden(NodoAVL *n, vector<NodoAVL *> &resultado) {
        if (!n) return;
        inorden(n->izquierdo, resultado);
        resultado.push_back(n);
        inorden(n->derecho, resultado);
    }

    void destruir(NodoAVL *n) {
        if (!n) return;
        destruir(n->izquierdo); destruir(n->derecho); delete n;
    }

public:
    ArbolAVL() : raiz(nullptr) {}
    ~ArbolAVL() { destruir(raiz); }

    void insertar(const string &pal, const string &trad = "", const string &id = "", int cont = 1) {
        raiz = insertar(raiz, pal, trad, id, cont); // Se permite el caso original (Mayúsculas)
    }

    bool eliminar(const string &pal) {
        if (!buscar(raiz, pal)) return false;
        raiz = eliminar(raiz, pal); return true;
    }

    NodoAVL *buscar(const string &pal) { return buscar(raiz, pal); }

    void incrementarContador(const string &pal) {
        NodoAVL *n = buscar(raiz, pal);
        if (n) n->contadorBusqueda++;
    }

    vector<NodoAVL *> obtenerTodos() {
        vector<NodoAVL *> resultado;
        inorden(raiz, resultado); return resultado;
    }

    void cargarDesdeArchivoEncriptado(const string &ruta) {
        ifstream archivo(ruta);
        if (!archivo.is_open()) return;
        string linea;
        while (getline(archivo, linea)) {
            if (linea.empty()) continue;
            string lineaDec = desencriptar(linea);
            size_t p1 = lineaDec.find('|');
            size_t p2 = lineaDec.find('|', p1 + 1);
            size_t p3 = lineaDec.find('|', p2 + 1);
            if (p1 == string::npos || p2 == string::npos || p3 == string::npos) continue;
            string pal = lineaDec.substr(0, p1);
            string trad = lineaDec.substr(p1 + 1, p2 - p1 - 1);
            string id = lineaDec.substr(p2 + 1, p3 - p2 - 1);
            int cont = stoi(lineaDec.substr(p3 + 1));
            insertar(pal, trad, id, cont);
        }
        archivo.close();
    }
};

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

string traducir(const string &texto, const string &lenguajeDestino) {
    CURL *curl; CURLcode res; string respuestaJson;
    curl = curl_easy_init();
    if (curl) {
        char *textoEscapado = curl_easy_escape(curl, texto.c_str(), (int)texto.length());
        string url = "https://translation.googleapis.com/language/translate/v2?q=" +
                     string(textoEscapado) + "&target=" + lenguajeDestino + "&key=" + MI_API_KEY;
        curl_free(textoEscapado);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respuestaJson);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res == CURLE_OK) {
            auto j = json::parse(respuestaJson);
            if (j.contains("data")) return j["data"]["translations"][0]["translatedText"];
        }
    }
    return "[Error de conexion]";
}

void mostrarSugerencias(ArbolAVL &arbol, const string &usuario) {
    auto todos = arbol.obtenerTodos();
    if (todos.empty()) return;
    sort(todos.begin(), todos.end(), [](NodoAVL *a, NodoAVL *b) { return a->contadorBusqueda > b->contadorBusqueda; });
    cout << "\n--- Sugerencias para " << usuario << " (mas buscadas) ---\n";
    int limite = min((int)todos.size(), 3);
    for (int i = 0; i < limite; i++)
        cout << " > " << todos[i]->palabra << " -> " << todos[i]->traduccion << " [" << todos[i]->idioma << "] (" << todos[i]->contadorBusqueda << " veces)\n";
    cout << "---------------------------------------------\n";
}

void mostrarHistorial(ArbolAVL &arbol) {
    auto todos = arbol.obtenerTodos();
    if (todos.empty()) { cout << "\n[Sin historial]\n"; return; }
    sort(todos.begin(), todos.end(), [](NodoAVL *a, NodoAVL *b) { return a->contadorBusqueda > b->contadorBusqueda; });
    cout << "\n--- Historial completo ---\n";
    cout << left << setw(20) << "Palabra" << setw(25) << "Traduccion" << setw(8) << "Idioma" << "Busquedas\n";
    for (NodoAVL *n : todos)
        cout << left << setw(20) << n->palabra << setw(25) << n->traduccion << setw(8) << n->idioma << n->contadorBusqueda << "\n";
}

void guardarLlave(const string &dir) {
    vector<pair<string, string>> pares(encryptMap.begin(), encryptMap.end());
    sort(pares.begin(), pares.end());
    ofstream fLlave(dir + "/llave.txt");
    fLlave << "Umg\n"; // Requerimiento Fase 2
    for (auto &p : pares) fLlave << p.first << " -> [" << p.second << "]\n";
    fLlave.close();
}

void guardarHistorialEncriptado(const string &usuario, ArbolAVL &arbol) {
    string dir = "usuarios/" + usuario;
    crearDirectorioRecursivo(dir);
    ofstream fOrig(dir + "/historial_original.txt");
    ofstream fCif(dir  + "/historial_cifrado.txt");
    for (NodoAVL *n : arbol.obtenerTodos()) {
        string linea = n->palabra + "|" + n->traduccion + "|" + n->idioma + "|" + to_string(n->contadorBusqueda);
        fOrig << linea << "\n";
        fCif  << encriptar(linea) << "\n";
    }
    fOrig.close(); fCif.close();
    guardarLlave(dir);
}

void pausar() { cout << "\nPresione ENTER para continuar..."; cin.ignore(); cin.get(); }

string gestionarUsuario() {
    string usuario; int opcion;
    while (true) {
        cout << "  TRADUCTOR UMG - Inicio!\n  1. Iniciar sesion\n  2. Registrar nuevo usuario\n  3. Salir\n  Opcion: ";
        cin >> opcion; if (opcion == 3) exit(0);
        cout << "ID de Usuario: "; cin >> usuario;
        string ruta = "usuarios/" + usuario + "/historial_cifrado.txt";
        if (opcion == 1) {
            ifstream archivo(ruta);
            if (archivo.good()) { archivo.close(); return usuario; }
            else cout << "[Usuario no encontrado]\n";
        } else if (opcion == 2) {
            string dir = "usuarios/" + usuario;
            crearDirectorioRecursivo(dir); return usuario;
        }
    }
}

int main() {
    string usuario = gestionarUsuario();
    string rutaCifrado = "usuarios/" + usuario + "/historial_cifrado.txt";
    ArbolAVL historial; historial.cargarDesdeArchivoEncriptado(rutaCifrado);

    int opcion;
    do {
        mostrarSugerencias(historial, usuario);
        cout << "   TRADUCTOR UMG  |  Usuario: " << usuario << "\n";
        cout << "  1. Traducir palabra o frase\n  2. Agregar traduccion manual\n  3. Eliminar del historial\n  4. Ver historial completo\n  5. Salir\n  Opcion: ";
        cin >> opcion; 
        switch (opcion) {
        case 1: {
            string palabra, idioma, resultado;
            cout << "Texto a traducir: "; cin.ignore(); getline(cin, palabra);
            cout << "Idioma destino (en, fr, it, de, es): "; cin >> idioma;
            NodoAVL *cache = historial.buscar(palabra);
            if (cache && cache->idioma == idioma) {
                resultado = cache->traduccion;
                cout << "\n[Cache AVL] TRADUCCION: " << resultado << "\n";
                historial.incrementarContador(palabra);
            } else {
                resultado = traducir(palabra, idioma);
                cout << "TRADUCCION: " << resultado << "\n";
                int contPrevio = cache ? cache->contadorBusqueda + 1 : 1;
                if (cache) historial.eliminar(palabra);
                historial.insertar(palabra, resultado, idioma, contPrevio);
            }
            reproducirAudio(resultado); pausar(); break;
        }
        case 2: {
            string palabra, traduccion, idioma;
            cout << "Texto original: "; cin.ignore(); getline(cin, palabra);
            cout << "Traduccion: "; getline(cin, traduccion);
            cout << "Idioma: "; cin >> idioma;
            historial.insertar(palabra, traduccion, idioma, 1);
            cout << "\n[Nodo insertado en el arbol AVL]\n"; pausar(); break;
        }
        case 3: {
            string palabra; cout << "Texto a eliminar: "; cin.ignore(); getline(cin, palabra);
            if (historial.eliminar(palabra)) cout << "\n[Nodo eliminado]\n";
            else cout << "\n[No encontrado]\n";
            pausar(); break;
        }
        case 4: mostrarHistorial(historial); pausar(); break;
        case 5: guardarHistorialEncriptado(usuario, historial); cout << "Sesion guardada.\n"; break;
        }
    } while (opcion != 5);
    return 0;
}