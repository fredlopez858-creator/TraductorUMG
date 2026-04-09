#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

const string MI_API_KEY = "AIzaSyBOfIq453ZzyFIDJXbFOPmP4CUthlGfGDs";


struct NodoAVL {
    string palabra;
    string traduccion;
    string idioma;
    int contadorBusqueda;
    int altura;
    NodoAVL* izquierdo;
    NodoAVL* derecho;

    NodoAVL(const string& pal, const string& trad, const string& id, int cont)
        : palabra(pal), traduccion(trad), idioma(id), contadorBusqueda(cont),
          altura(1), izquierdo(nullptr), derecho(nullptr) {}
};

class ArbolAVL {
private:
    NodoAVL* raiz;

    int altura(NodoAVL* n) {
        return n ? n->altura : 0;
    }

    int factorBalance(NodoAVL* n) {
        return n ? altura(n->izquierdo) - altura(n->derecho) : 0;
    }

    void actualizarAltura(NodoAVL* n) {
        if (n)
            n->altura = 1 + max(altura(n->izquierdo), altura(n->derecho));
    }

    NodoAVL* rotarDerecha(NodoAVL* y) {
        NodoAVL* x  = y->izquierdo;
        NodoAVL* T2 = x->derecho;
        x->derecho  = y;
        y->izquierdo = T2;
        actualizarAltura(y);
        actualizarAltura(x);
        return x;
    }

    NodoAVL* rotarIzquierda(NodoAVL* x) {
        NodoAVL* y  = x->derecho;
        NodoAVL* T2 = y->izquierdo;
        y->izquierdo = x;
        x->derecho   = T2;
        actualizarAltura(x);
        actualizarAltura(y);
        return y;
    }

    NodoAVL* balancear(NodoAVL* n) {
        actualizarAltura(n);
        int fb = factorBalance(n);

        if (fb > 1 && factorBalance(n->izquierdo) >= 0)
            return rotarDerecha(n);

        if (fb > 1 && factorBalance(n->izquierdo) < 0) {
            n->izquierdo = rotarIzquierda(n->izquierdo);
            return rotarDerecha(n);
        }

        if (fb < -1 && factorBalance(n->derecho) <= 0)
            return rotarIzquierda(n);

        if (fb < -1 && factorBalance(n->derecho) > 0) {
            n->derecho = rotarDerecha(n->derecho);
            return rotarIzquierda(n);
        }

        return n;
    }

    NodoAVL* insertar(NodoAVL* n, const string& pal, const string& trad,
                      const string& id, int cont) {
        if (!n) return new NodoAVL(pal, trad, id, cont);

        if (pal < n->palabra)
            n->izquierdo = insertar(n->izquierdo, pal, trad, id, cont);
        else if (pal > n->palabra)
            n->derecho = insertar(n->derecho, pal, trad, id, cont);
        else {
            n->traduccion      = trad;
            n->idioma          = id;
            n->contadorBusqueda = cont;
            return n;
        }

        return balancear(n);
    }

    NodoAVL* minimoNodo(NodoAVL* n) {
        while (n->izquierdo) n = n->izquierdo;
        return n;
    }

    NodoAVL* eliminar(NodoAVL* n, const string& pal) {
        if (!n) return nullptr;

        if (pal < n->palabra)
            n->izquierdo = eliminar(n->izquierdo, pal);
        else if (pal > n->palabra)
            n->derecho = eliminar(n->derecho, pal);
        else {
            if (!n->izquierdo || !n->derecho) {
                NodoAVL* temp = n->izquierdo ? n->izquierdo : n->derecho;
                delete n;
                return temp;
            }
            NodoAVL* suc     = minimoNodo(n->derecho);
            n->palabra       = suc->palabra;
            n->traduccion    = suc->traduccion;
            n->idioma        = suc->idioma;
            n->contadorBusqueda = suc->contadorBusqueda;
            n->derecho       = eliminar(n->derecho, suc->palabra);
        }

        return balancear(n);
    }

    NodoAVL* buscar(NodoAVL* n, const string& pal) {
        if (!n || n->palabra == pal) return n;
        if (pal < n->palabra) return buscar(n->izquierdo, pal);
        return buscar(n->derecho, pal);
    }

    void inorden(NodoAVL* n, vector<NodoAVL*>& resultado) {
        if (!n) return;
        inorden(n->izquierdo, resultado);
        resultado.push_back(n);
        inorden(n->derecho, resultado);
    }

    void destruir(NodoAVL* n) {
        if (!n) return;
        destruir(n->izquierdo);
        destruir(n->derecho);
        delete n;
    }

public:
    ArbolAVL() : raiz(nullptr) {}
    ~ArbolAVL() { destruir(raiz); }

    void insertar(const string& pal, const string& trad = "", const string& id = "", int cont = 1) {
        raiz = insertar(raiz, pal, trad, id, cont);
    }

    bool eliminar(const string& pal) {
        if (!buscar(raiz, pal)) return false;
        raiz = eliminar(raiz, pal);
        return true;
    }

    NodoAVL* buscar(const string& pal) {
        return buscar(raiz, pal);
    }

    void incrementarContador(const string& pal) {
        NodoAVL* n = buscar(raiz, pal);
        if (n) n->contadorBusqueda++;
    }

    vector<NodoAVL*> obtenerTodos() {
        vector<NodoAVL*> resultado;
        inorden(raiz, resultado);
        return resultado;
    }

    bool estaVacio() { return raiz == nullptr; }

    // Formato de archivo: palabra|traduccion|idioma|contador
    void cargarDesdeArchivo(const string& ruta) {
        ifstream archivo(ruta);
        if (!archivo.is_open()) return;
        string linea;
        while (getline(archivo, linea)) {
            if (linea.empty()) continue;
            size_t p1 = linea.find('|');
            size_t p2 = linea.find('|', p1 + 1);
            size_t p3 = linea.find('|', p2 + 1);
            if (p1 == string::npos || p2 == string::npos || p3 == string::npos) continue;
            string pal  = linea.substr(0, p1);
            string trad = linea.substr(p1 + 1, p2 - p1 - 1);
            string id   = linea.substr(p2 + 1, p3 - p2 - 1);
            int cont    = stoi(linea.substr(p3 + 1));
            insertar(pal, trad, id, cont);
        }
        archivo.close();
    }

    void guardarEnArchivo(const string& ruta) {
        ofstream archivo(ruta);
        for (NodoAVL* n : obtenerTodos()) {
            archivo << n->palabra << "|"
                    << n->traduccion << "|"
                    << n->idioma << "|"
                    << n->contadorBusqueda << "\n";
        }
        archivo.close();
    }
};


size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string traducir(const string& texto, const string& lenguajeDestino) {
    CURL* curl;
    CURLcode res;
    string respuestaJson;

    curl = curl_easy_init();
    if (curl) {
        char* textoEscapado = curl_easy_escape(curl, texto.c_str(), texto.length());
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
            if (j.contains("data")) {
                return j["data"]["translations"][0]["translatedText"];
            }
        }
    }
    return "[Error de conexion o API Key invalida]";
}

// FUNCIONES DE INTERFAZ y UTILIDADES

void mostrarSugerencias(ArbolAVL& arbol, const string& usuario) {
    auto todos = arbol.obtenerTodos();
    if (todos.empty()) return;

    sort(todos.begin(), todos.end(), [](NodoAVL* a, NodoAVL* b) {
        return a->contadorBusqueda > b->contadorBusqueda;
    });

    cout << "\n--- Sugerencias para " << usuario << " (mas buscadas) ---\n";
    int limite = min((int)todos.size(), 3);
    for (int i = 0; i < limite; i++) {
        cout << " > " << todos[i]->palabra << " -> " << todos[i]->traduccion
             << " [" << todos[i]->idioma << "] ("
             << todos[i]->contadorBusqueda << " veces)\n";
    }
    cout << "---------------------------------------------\n";
}

void mostrarHistorial(ArbolAVL& arbol) {
    auto todos = arbol.obtenerTodos();
    if (todos.empty()) {
        cout << "\n[Sin historial de busquedas]\n";
        return;
    }

    sort(todos.begin(), todos.end(), [](NodoAVL* a, NodoAVL* b) {
        return a->contadorBusqueda > b->contadorBusqueda;
    });

    cout << "\n--- Historial completo (por frecuencia) ---\n";
    cout << left << setw(20) << "Palabra"
                 << setw(25) << "Traduccion"
                 << setw(8)  << "Idioma"
                 << "Busquedas\n";
    cout << string(60, '-') << "\n";
    for (NodoAVL* n : todos) {
        cout << left << setw(20) << n->palabra
                     << setw(25) << n->traduccion
                     << setw(8)  << n->idioma
                     << n->contadorBusqueda << "\n";
    }
    cout << string(60, '-') << "\n";
}

//  FUNCIONES MINIMAS PARAMAIN 

void pausar() {
    cout << "\nPresione ENTER para continuar...";
    cin.ignore();
    cin.get();
}

string gestionarUsuario() {
    string usuario;
    int opcion;

    while (true) {
        cout << "  TRADUCTOR UMG - Inicio!"<<endl;
        cout << "  1. Iniciar sesion"<<endl;
        cout << "  2. Registrar nuevo usuario"<<endl;
        cout << "  3. Salir"<<endl;
        cout << "  Opcion: ";
        cin >> opcion;

        switch (opcion) {
            case 1: {
                cout << "INICIAR SESION"<<endl;
                cout << "ID de Usuario: "<<endl;
                cin >> usuario;

                string ruta = "sesiones/" + usuario + ".txt";
                ifstream archivo(ruta);
                if (archivo.good()) {
                    archivo.close();
                    cout << "\n[Bienvenido de nuevo, " << usuario << "!]\n";
                    pausar();
                    return usuario;
                } else {
                    cout << "\n[Usuario no encontrado. Registrese primero.]\n";
                    pausar();
                }
                break;
            }
            case 2: {
                cout << "REGISTRAR USUARIO"<<endl;;
                cout << "Nuevo ID de Usuario: ";
                cin >> usuario;

                string ruta = "sesiones/" + usuario + ".txt";
                ifstream verificar(ruta);
                if (verificar.good()) {
                    verificar.close();
                    cout << "\n[El usuario '" << usuario << "' ya existe. Inicie sesion.]\n";
                    pausar();
                } else {
                    ofstream nuevo(ruta);
                    nuevo.close();
                    cout << "\n[Usuario '" << usuario << "' registrado exitosamente!]\n";
                    pausar();
                    return usuario;
                }
                break;
            }
            case 3: {
                cout << "Hasta luego!"<<endl;
                exit(0);
            }
            default: {
                cout << "[Opcion invalida]"<<endl;
                pausar();
                break;
            }
        }
    }
}

int main() {
    string usuario = gestionarUsuario();

    string ruta = "sesiones/" + usuario + ".txt";
    ArbolAVL historial;
    historial.cargarDesdeArchivo(ruta);

    int opcion;
    do {
        mostrarSugerencias(historial, usuario);

        cout << "   TRADUCTOR UMG  |  Usuario: " << usuario << "\n";
        cout << "  1. Traducir palabra"<<endl;
        cout << "  2. Agregar palabra con traduccion manual"<<endl;
        cout << "  3. Eliminar palabra del historial"<<endl;
        cout << "  4. Ver historial completo"<<endl;
        cout << "  5. Salir"<<endl;
        cout << "  Opcion: ";
        cin >> opcion;

        switch (opcion) {
            case 1: {
                string palabra, idioma;
                cout << "TRADUCIR PALABRA"<<endl;
                cout << "Palabra a traducir: "<<endl;
                cin >> palabra;
                cout << "Idioma destino (en, fr, it, de, es): "<<endl;
                cin >> idioma;

                NodoAVL* cache = historial.buscar(palabra);
                if (cache && cache->idioma == idioma) {
                    cout << "\n[Cache AVL] TRADUCCION: " << cache->traduccion << "\n";
                    historial.incrementarContador(palabra);
                } else {
                    string resultado = traducir(palabra, idioma);
                    cout << "TRADUCCION: " << resultado << "\n";
                    int contPrevio = cache ? cache->contadorBusqueda + 1 : 1;
                    if (cache) historial.eliminar(palabra);
                    historial.insertar(palabra, resultado, idioma, contPrevio);
                }
                pausar();
                break;
            }
            case 2: {
                string palabra, traduccion, idioma;
                cout << "AGREGAR PALABRA MANUAL"<<endl;
                cout << "Palabra original: "<<endl;
                cin >> palabra;
                cout << "Traduccion: "<<endl;
                cin >> traduccion;
                cout << "Idioma (en, fr, it, de, es): "<<endl;
                cin >> idioma;
                historial.insertar(palabra, traduccion, idioma, 1);
                cout << "\n[Nodo insertado en el arbol AVL]\n";
                pausar();
                break;
            }
            case 3: {
                string palabra;
                cout << "ELIMINAR DEL HISTORIAL"<<endl;
                cout << "Palabra a eliminar: "<<endl;
                cin >> palabra;
                if (historial.eliminar(palabra))
                    cout << "\n[Nodo eliminado del arbol AVL]\n";
                else
                    cout << "\n[Palabra no encontrada en el historial]\n";
                pausar();
                break;
            }
            case 4: {
                mostrarHistorial(historial);
                pausar();
                break;
            }
            case 5: {
                historial.guardarEnArchivo(ruta);
                cout << "Sesion guardada. Hasta luego, " << usuario << "!"<<endl;
                break;
            }
            default: {
                cout << "[Opcion invalida. Intente de nuevo]"<<endl;
                pausar();
                break;
            }
        }

    } while (opcion != 5);

    return 0;
}
