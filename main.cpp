#include <windows.h>
#include <string.h>
#include <atomic>
#include "plutonium_sdk.hpp"  // SDK oficial de Plutonium para crear plugins
#include "MinHook.h"          // Librería para interceptar funciones del juego

// ─────────────────────────────────────────────
// ESTRUCTURAS DEL JUEGO
// ─────────────────────────────────────────────

struct snd_alias_t
{
    const char* name;
};

struct SndStartAliasInfo
{
    const snd_alias_t* alias;
};

// ─────────────────────────────────────────────
// FUNCIONES DEL JUEGO
// ─────────────────────────────────────────────

typedef void(__cdecl* Cbuf_AddText_t)(int client_num, const char* text);
Cbuf_AddText_t Cbuf_AddText = (Cbuf_AddText_t)0x49B930;

typedef int(__cdecl* SD_StartAlias_t)(SndStartAliasInfo* startAliasInfo, unsigned int voice);
SD_StartAlias_t original_SD_StartAlias = nullptr;

// ─────────────────────────────────────────────
// ESTADO DEL PLUGIN
// ─────────────────────────────────────────────

// Contador de la ronda actual (acceso thread-safe)
static std::atomic<int> ronda_actual{ 1 };

static std::atomic<bool> partida_terminada{ false };

// ─────────────────────────────────────────────
// ALIASES DE SONIDO VIGILADOS
// ─────────────────────────────────────────────

static const char* const ALIASES_FIN_RONDA[] =
{
    "mus_zombie_round_over",        // Música estándar de fin de ronda
    "mus_zombie_dog_end",           // Fin de ronda de perros
    "zmb_vox_pentann_thiefend_bad", // Fin de ronda de FIVE (resultado malo)
    "zmb_vox_pentann_thiefend_good" // Fin de ronda de FIVE (resultado bueno)
};
static constexpr int NUM_ALIASES = sizeof(ALIASES_FIN_RONDA) / sizeof(ALIASES_FIN_RONDA[0]);

static const char* const ALIASES_FIN_PARTIDA[] =
{
    "mus_zombie_game_over"
};
static constexpr int NUM_ALIASES_FIN_PARTIDA = sizeof(ALIASES_FIN_PARTIDA) / sizeof(ALIASES_FIN_PARTIDA[0]);

// ─────────────────────────────────────────────
// CALLBACKS DE CICLO DE VIDA
// ─────────────────────────────────────────────

static void PLUTONIUM_CALLBACK al_iniciar_juego(int param1, int param2)
{
    ronda_actual.store(1);
    partida_terminada.store(false);
}

static void PLUTONIUM_CALLBACK al_cerrar_juego(int param)
{
    ronda_actual.store(1);
    partida_terminada.store(false);
}

// ─────────────────────────────────────────────
// LÓGICA DE VERIFICACIÓN
// ─────────────────────────────────────────────
static void verificar(int ronda)
{
    bool debe_verificar = false;
    bool debe_verificar_todo = false;

    // Verificación periódica según tramo de ronda
    if (ronda >= 200)
    {
        // A partir de ronda 200: cada 5 rondas (200, 205, 210...)
        if (ronda % 5 == 0)
            debe_verificar = true;
    }
    else
    {
        // Antes de ronda 200: cada 10 rondas (10, 20, 30...)
        if (ronda % 10 == 0)
            debe_verificar = true;
    }

    if (ronda == 3 || ronda == 28 || ronda == 48 || ronda == 68 || ronda == 98 || ronda == 148 || ronda == 198)
    {
        debe_verificar_todo = true;
    }

    if (debe_verificar_todo)
    {
        // Envía el comando que hace que el juego
        // reporte los hashes de sus archivos,
        // permitiendo verificar su integridad
        Cbuf_AddText(0, "flashScriptHashes 2");
    }
    else if (debe_verificar)
    {
        Cbuf_AddText(0, "flashScriptHashes");
    }
}

// Incrementa el contador de ronda y lanza
// la verificación si corresponde
static void avanzar_ronda()
{
    int nueva_ronda = ++ronda_actual;
    verificar(nueva_ronda);
}

// ─────────────────────────────────────────────
// PROCESAMIENTO DE FIN DE PARTIDA
// ─────────────────────────────────────────────
static void procesar_fin_de_partida()
{
    bool esperado = false;
    if (!partida_terminada.compare_exchange_strong(esperado, true))
        return;

    // Flash completo de todos los hashes al terminar la partida.
    // Esto cubre el caso en que un cheat se inyectó justo antes
    // del game over y aún no había sido detectado por los checks
    // periódicos de ronda.
    Cbuf_AddText(0, "flashScriptHashes");
}

// ─────────────────────────────────────────────
// HOOK DE SONIDO
// ─────────────────────────────────────────────
static int __cdecl hook_SD_StartAlias(SndStartAliasInfo* startAliasInfo, unsigned int voice)
{
    if (startAliasInfo != nullptr &&
        startAliasInfo->alias != nullptr &&
        startAliasInfo->alias->name != nullptr)
    {
        const char* alias_name = startAliasInfo->alias->name;

        // Detección de fin de ronda
        for (int i = 0; i < NUM_ALIASES; ++i)
        {
            if (strcmp(alias_name, ALIASES_FIN_RONDA[i]) == 0)
            {
                avanzar_ronda();
                break;
            }
        }

        // Detección de game over
        for (int i = 0; i < NUM_ALIASES_FIN_PARTIDA; ++i)
        {
            if (strcmp(alias_name, ALIASES_FIN_PARTIDA[i]) == 0)
            {
                procesar_fin_de_partida();
                break;
            }
        }
    }

    // Llamamos siempre a la función original: el sonido debe reproducirse
    return original_SD_StartAlias(startAliasInfo, voice);
}

// ─────────────────────────────────────────────
// CLASE PRINCIPAL DEL PLUGIN
// ─────────────────────────────────────────────
class PluginVerificador : public plutonium::sdk::plugin
{
public:
    const char* plugin_name() override
    {
        return "Sistema de Verificacion T5";
    }

    bool is_game_supported(plutonium::sdk::game game) override
    {
        return game == plutonium::sdk::game::t5;
    }

    void on_startup(plutonium::sdk::iinterface* interface_ptr, plutonium::sdk::game game) override
    {
		ronda_actual.store(1);          // Reiniciamos el contador de ronda
		partida_terminada.store(false); // Reiniciamos el flag de partida terminada

        // Registramos los callbacks de inicio y cierre de partida
        if (interface_ptr != nullptr && interface_ptr->callbacks() != nullptr)
        {
            interface_ptr->callbacks()->on_game_init(al_iniciar_juego);
            interface_ptr->callbacks()->on_game_shutdown(al_cerrar_juego);
        }

        // Inicializamos MinHook (motor de hooking)
        if (MH_Initialize() != MH_OK)
            return;
        // Creamos el hook: redirigimos SD_StartAlias (en 0x5642B0)
        // a nuestra función hook_SD_StartAlias y guardamos la original
        if (MH_CreateHook((LPVOID)0x5642B0,
            &hook_SD_StartAlias,
            (LPVOID*)&original_SD_StartAlias) != MH_OK)
        {
            MH_Uninitialize();
            return;
        }
        // Activamos el hook (a partir de aquí el juego
        // pasará por nuestra función al reproducir sonidos)
        if (MH_EnableHook((LPVOID)0x5642B0) != MH_OK)
        {
            MH_RemoveHook((LPVOID)0x5642B0);
            MH_Uninitialize();
            return;
        }
    }

    void on_shutdown() override
    {
        MH_DisableHook((LPVOID)0x5642B0);  // Desactivamos el hook
        MH_RemoveHook((LPVOID)0x5642B0);   // Eliminamos el hook
        MH_Uninitialize();                  // Liberamos MinHook
    }
};

// ─────────────────────────────────────────────
// INICIALIZACIÓN Y PUNTO DE ENTRADA
// ─────────────────────────────────────────────

static PluginVerificador g_plugin_instance;

// Función que el SDK llama para obtener el plugin.
// Exportada como C para evitar name mangling de C++.
extern "C" __declspec(dllexport) plutonium::sdk::plugin* __cdecl on_initialize()
{
    return &g_plugin_instance;
}

// Punto de entrada estándar de toda DLL en Windows.
// No necesitamos hacer nada especial aquí.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}