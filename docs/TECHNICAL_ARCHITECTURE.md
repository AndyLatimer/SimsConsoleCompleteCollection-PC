# Arquitectura técnica

Se conserva la arquitectura de sims1-recomp: el XBE x86 se traduce estáticamente a C++20;
el ejecutable Windows x64 usa Direct3D 12, XAudio2 y SDL3. No se usa un emulador externo.
El código generado queda en `generated/game/`, fuera de Git, y nunca se edita a mano.

| Ubicación | Responsabilidad |
| --- | --- |
| `src/common/` | GameInfo, mando/teclado y paneles F1/F2 |
| `src/game/` | Hooks USA de tiempo, zoom, callback y trucos |
| `config/game.toml` | Direcciones y parámetros del XBE USA, 32 particiones |
| `sdk/` | Runtime y recompilador, submódulo fijado |
| `sdk/xdk/5233/` | Firmas y layout específicos, distintos de otras versiones XDK |
| `assets/` | Copia privada de datos del usuario, ignorada en Git |
| `out/build/` | Compilaciones, dependencias y herramientas generadas |
| `collection/game.json` | Contrato inicial de integración, sin launcher |
| `tools/game_data.py` | Importación, inspección y comprobación de disponibilidad |

Revisiones exactas en `PROJECT_STATUS.md`. CMake exige el SDK local; no cae a un checkout
vecino ni descarga la rama principal. Se conserva el gitlink original.

Los hooks del juego corrigen la cuantización del paso temporal con reloj monotónico y el
zoom por frecuencia de llamadas. `D3DDevice_InsertCallback` ejecuta el callback huésped que
desbloquea el primer cuadro. El SDK incorpora despertares dirigidos y spin limitado de
secciones críticas; no se han reemplazado esas soluciones.

El importador comprueba SHA-1 del XBE y tamaños de los recursos conocidos. Estos tamaños
no garantizan integridad del contenido: para comparar USA/PAL se generan SHA-256, y para
declarar compatibilidad hace falta ejecución. Copia a una carpeta temporal, revalida y
solo entonces publica `assets/`; rechaza destinos existentes y conserva el origen.

Licencias: `sdk/LICENSE` es MIT (TGP482); avisos en `sdk/THIRD_PARTY.txt`.
El repositorio padre no incluye LICENSE para su código específico: no se le atribuye una
licencia inventada. Se mantienen historial, créditos y avisos. Resolver esa autorización
antes de distribuir el código ajeno o un paquete final. No se incluyen datos comerciales.
