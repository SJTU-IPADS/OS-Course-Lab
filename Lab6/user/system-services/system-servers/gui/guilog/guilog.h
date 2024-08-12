#include <g2d/g2d.h>
#define GUI_GUI_DEBUG 0
#if GUI_GUI_DEBUG
#define GUI_DEBUG(...)            (printf(__VA_ARGS__))
#define GUI_DEBUG_PRINT_REGION(x) (print_region(x))
#define GUI_DEBUG_PRINT_RECT(x)   (print_rect(x))
void save_window(PBITMAP bitmap, PRGN region);
#else
#define GUI_DEBUG(...) \
        {          \
        }
#define GUI_DEBUG_PRINT_REGION(x) \
        {                     \
        }
#define GUI_DEBUG_PRINT_RECT(x) \
        {                   \
        }
#endif

#define GUI_WARN(...) (printf("[GUI WARN] %s:%d ", __FILE__, __LINE__), printf(__VA_ARGS__))
#define GUI_ERROR(...) (printf("[GUI ERROR] %s:%d ", __FILE__, __LINE__), printf(__VA_ARGS__))
#define GUI_LOG(...) (printf("[GUI] "), printf(__VA_ARGS__))
