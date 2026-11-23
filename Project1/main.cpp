#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <set>
#include <utility>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

enum Herramienta {
    SELECCIONAR,
    PINCEL_GRILLA,
    LINEA,
    CUADRADO,
    CIRCULO,
    TRIANGULO,
    BOTEPINTURA,
    BORRADOR
};

using Celda = std::pair<int, int>; // (col, fila)

struct Figura {
    Herramienta tipo = LINEA;
    // Coordenadas en unidades de celda de grilla
    float cx = 0.0f, cy = 0.0f;
    float hw = 0.0f, hh = 0.0f;
    float angulo = 0.0f;
    float borde[3] = { 0.08f, 0.40f, 0.75f }; // Azul estilo la imagen por defecto
    float relleno[3] = { 1.0f, 0.8f, 0.2f };
    bool rellena = false;
};

// Estado de la aplicación
std::vector<Figura> figuras;
int seleccion = -1;
Herramienta herramientaActual = CIRCULO;

// Configuración por defecto
float colorBorde[3] = { 0.08f, 0.40f, 0.75f };
float colorRelleno[3] = { 1.0f, 0.8f, 0.2f };
bool rellenarNuevas = false;

float colorFondo[3] = { 1.0f, 1.0f, 1.0f };
float tamGrilla = 15.0f;

// Estado del ratón (0 = libre, 1 = dibujando, 2 = moviendo)
int modo = 0;
int inicioCol = 0, inicioFila = 0;
int actualCol = 0, actualFila = 0;
float offsetCol = 0.0f, offsetFila = 0.0f;

const float PI = 3.14159265f;

// --- Utilidades de Grilla ---

float Normalizar(float a) {
    a = std::fmod(a, 360.0f);
    return (a < 0.0f) ? a + 360.0f : a;
}

int PantallaACol(float x) { return (int)std::floor(x / tamGrilla); }
int PantallaAFila(float y) { return (int)std::floor(y / tamGrilla); }

// --- Rasterización en Celdas (Bresenham / Punto Medio) ---

void RasterizarLinea(int x0, int y0, int x1, int y1, std::set<Celda>& celdas) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        celdas.insert({ x0, y0 });
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void RasterizarCirculo(int cx, int cy, int r, bool rellena, std::set<Celda>& borde, std::set<Celda>& interior) {
    if (r <= 0) {
        borde.insert({ cx, cy });
        return;
    }
    int x = r, y = 0;
    int err = 1 - r;

    auto agregarOctantes = [&](int px, int py) {
        borde.insert({ cx + px, cy + py });
        borde.insert({ cx - px, cy + py });
        borde.insert({ cx + px, cy - py });
        borde.insert({ cx - px, cy - py });
        borde.insert({ cx + py, cy + px });
        borde.insert({ cx - py, cy + px });
        borde.insert({ cx + py, cy - px });
        borde.insert({ cx - py, cy - px });

        if (rellena) {
            for (int ix = cx - px; ix <= cx + px; ix++) {
                interior.insert({ ix, cy + py });
                interior.insert({ ix, cy - py });
            }
            for (int ix = cx - py; ix <= cx + py; ix++) {
                interior.insert({ ix, cy + px });
                interior.insert({ ix, cy - px });
            }
        }
        };

    while (x >= y) {
        agregarOctantes(x, y);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        }
        else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

// Transforma un punto local (en celdas) a celda entera del mundo aplicando rotación
Celda RotarACelda(const Figura& f, float lx, float ly) {
    float rad = f.angulo * PI / 180.0f;
    float c = std::cos(rad);
    float s = std::sin(rad);
    float wx = f.cx + (lx * c - ly * s);
    float wy = f.cy + (lx * s + ly * c);
    return { (int)std::round(wx), (int)std::round(wy) };
}

// Obtiene todas las celdas exactas que ocupa una figura en la grilla
void ObtenerCeldasFigura(const Figura& f, std::set<Celda>& celdasBorde, std::set<Celda>& celdasRelleno) {
    celdasBorde.clear();
    celdasRelleno.clear();

    if (f.tipo == PINCEL_GRILLA) {
        celdasRelleno.insert({ (int)std::round(f.cx), (int)std::round(f.cy) });
        return;
    }

    if (f.tipo == LINEA) {
        Celda p0 = RotarACelda(f, -f.hw, -f.hh);
        Celda p1 = RotarACelda(f, f.hw, f.hh);
        RasterizarLinea(p0.first, p0.second, p1.first, p1.second, celdasBorde);
        return;
    }

    if (f.tipo == CIRCULO) {
        int r = (int)std::round(std::fabs(f.hw));
        RasterizarCirculo((int)std::round(f.cx), (int)std::round(f.cy), r, f.rellena, celdasBorde, celdasRelleno);
        return;
    }

    // Polígonos (CUADRADO o TRIANGULO)
    std::vector<Celda> verts;
    if (f.tipo == CUADRADO) {
        verts.push_back(RotarACelda(f, -f.hw, -f.hh));
        verts.push_back(RotarACelda(f, f.hw, -f.hh));
        verts.push_back(RotarACelda(f, f.hw, f.hh));
        verts.push_back(RotarACelda(f, -f.hw, f.hh));
    }
    else if (f.tipo == TRIANGULO) {
        verts.push_back(RotarACelda(f, 0.0f, -f.hh));
        verts.push_back(RotarACelda(f, f.hw, f.hh));
        verts.push_back(RotarACelda(f, -f.hw, f.hh));
    }

    for (size_t i = 0; i < verts.size(); i++) {
        Celda a = verts[i];
        Celda b = verts[(i + 1) % verts.size()];
        RasterizarLinea(a.first, a.second, b.first, b.second, celdasBorde);
    }

    // Relleno de polígono por scanline en celdas
    if (f.rellena && verts.size() >= 3) {
        int minY = verts[0].second, maxY = verts[0].second;
        for (const auto& v : verts) {
            minY = std::min(minY, v.second);
            maxY = std::max(maxY, v.second);
        }
        for (int y = minY; y <= maxY; y++) {
            std::vector<float> nodosX;
            for (size_t i = 0; i < verts.size(); i++) {
                Celda p1 = verts[i];
                Celda p2 = verts[(i + 1) % verts.size()];
                if ((p1.second < y && p2.second >= y) || (p2.second < y && p1.second >= y)) {
                    float x = p1.first + (float)(y - p1.second) / (p2.second - p1.second) * (p2.first - p1.first);
                    nodosX.push_back(x);
                }
            }
            std::sort(nodosX.begin(), nodosX.end());
            for (size_t i = 0; i + 1 < nodosX.size(); i += 2) {
                int startX = (int)std::ceil(nodosX[i]);
                int endX = (int)std::floor(nodosX[i + 1]);
                for (int x = startX; x <= endX; x++) {
                    celdasRelleno.insert({ x, y });
                }
            }
        }
    }
}

bool CrearFigura(Herramienta t, int sc, int sf, int ec, int ef, Figura& f) {
    f = Figura();
    f.tipo = t;
    f.rellena = rellenarNuevas;
    std::copy(colorBorde, colorBorde + 3, f.borde);
    std::copy(colorRelleno, colorRelleno + 3, f.relleno);

    int dc = ec - sc;
    int df = ef - sf;

    if (t == LINEA) {
        if (dc == 0 && df == 0) return false;
        f.cx = (sc + ec) * 0.5f;
        f.cy = (sf + ef) * 0.5f;
        f.hw = dc * 0.5f;
        f.hh = df * 0.5f;
        return true;
    }
    if (t == CUADRADO || t == CIRCULO) {
        int lado = std::max(std::abs(dc), std::abs(df));
        if (lado < 1) return false;
        int ec2 = sc + (dc < 0 ? -lado : lado);
        int ef2 = sf + (df < 0 ? -lado : df == 0 ? lado : lado);
        f.cx = (sc + ec2) * 0.5f;
        f.cy = (sf + ef2) * 0.5f;
        f.hw = f.hh = lado * 0.5f;
        return true;
    }
    if (t == TRIANGULO) {
        int ancho = std::abs(dc);
        int alto = std::abs(df);
        if (ancho < 1 || alto < 1) return false;
        int ec2 = sc + (dc < 0 ? -ancho : ancho);
        int ef2 = sf + (df < 0 ? -alto : alto);
        f.cx = (sc + ec2) * 0.5f;
        f.cy = (sf + ef2) * 0.5f;
        f.hw = ancho * 0.5f;
        f.hh = alto * 0.5f;
        return true;
    }
    return false;
}

// --- Selección y Transformaciones en Grilla ---

bool ContieneCelda(const Figura& f, int col, int fila) {
    std::set<Celda> borde, relleno;
    ObtenerCeldasFigura(f, borde, relleno);
    Celda objetivo = { col, fila };
    if (borde.count(objetivo) || relleno.count(objetivo)) return true;

    // Si la figura no está rellena, permitir seleccionarla haciendo clic dentro de su área interior
    if (!f.rellena && f.tipo != LINEA && f.tipo != PINCEL_GRILLA) {
        Figura temp = f;
        temp.rellena = true;
        ObtenerCeldasFigura(temp, borde, relleno);
        return relleno.count(objetivo) > 0;
    }
    return false;
}

int BuscarFigura(int col, int fila) {
    for (int i = (int)figuras.size() - 1; i >= 0; i--) {
        if (ContieneCelda(figuras[i], col, fila)) return i;
    }
    return -1;
}

void EliminarFigura(int i) {
    if (i < 0 || i >= (int)figuras.size()) return;
    figuras.erase(figuras.begin() + i);
    if (seleccion == i) seleccion = -1;
    else if (seleccion > i) seleccion--;
}

void EscalarEnGrilla(Figura& f, int deltaCeldas) {
    if (f.tipo == PINCEL_GRILLA) return;

    if (f.tipo == LINEA) {
        float sx = (f.hw < 0) ? -1.0f : 1.0f;
        float sy = (f.hh < 0) ? -1.0f : 1.0f;
        float paso = 0.5f * deltaCeldas;
        if (std::fabs(f.hw) > 0.0f) f.hw = sx * std::max(0.5f, std::fabs(f.hw) + paso);
        if (std::fabs(f.hh) > 0.0f) f.hh = sy * std::max(0.5f, std::fabs(f.hh) + paso);
        return;
    }

    float paso = 0.5f * deltaCeldas;
    float nuevoHw = std::max(0.5f, std::fabs(f.hw) + paso);
    float nuevoHh = std::max(0.5f, std::fabs(f.hh) + paso);

    // Mantener la esquina superior izquierda anclada a celdas enteras
    float esqCol = std::round(f.cx - nuevoHw);
    float esqFila = std::round(f.cy - nuevoHh);
    f.hw = nuevoHw;
    f.hh = nuevoHh;
    f.cx = esqCol + f.hw;
    f.cy = esqFila + f.hh;
}

void PintarCeldaGrilla(int col, int fila) {
    for (int i = (int)figuras.size() - 1; i >= 0; i--) {
        if (figuras[i].tipo == PINCEL_GRILLA &&
            (int)std::round(figuras[i].cx) == col &&
            (int)std::round(figuras[i].cy) == fila) {
            std::copy(colorRelleno, colorRelleno + 3, figuras[i].relleno);
            return;
        }
    }

    Figura celda;
    celda.tipo = PINCEL_GRILLA;
    celda.cx = (float)col;
    celda.cy = (float)fila;
    celda.rellena = true;
    std::copy(colorRelleno, colorRelleno + 3, celda.relleno);
    figuras.push_back(celda);
}

// --- Renderizado ---

void DibujarCelda(int col, int fila, const float color[3]) {
    float x0 = col * tamGrilla;
    float y0 = fila * tamGrilla;
    glColor3fv(color);
    glBegin(GL_QUADS);
    glVertex2f(x0, y0);
    glVertex2f(x0 + tamGrilla, y0);
    glVertex2f(x0 + tamGrilla, y0 + tamGrilla);
    glVertex2f(x0, y0 + tamGrilla);
    glEnd();
}

void DibujarGrilla(int ancho, int alto) {
    float lum = 0.299f * colorFondo[0] + 0.587f * colorFondo[1] + 0.114f * colorFondo[2];
    float k = (lum > 0.5f) ? -1.0f : 1.0f;

    float suave[3], fuerte[3];
    for (int i = 0; i < 3; i++) {
        suave[i] = std::clamp(colorFondo[i] + k * 0.10f, 0.0f, 1.0f);
        fuerte[i] = std::clamp(colorFondo[i] + k * 0.22f, 0.0f, 1.0f);
    }

    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int n = 0; n * tamGrilla <= ancho; n++) {
        float x = n * tamGrilla;
        glColor3fv((n % 5 == 0) ? fuerte : suave);
        glVertex2f(x, 0.0f);
        glVertex2f(x, (float)alto);
    }
    for (int n = 0; n * tamGrilla <= alto; n++) {
        float y = n * tamGrilla;
        glColor3fv((n % 5 == 0) ? fuerte : suave);
        glVertex2f(0.0f, y);
        glVertex2f((float)ancho, y);
    }
    glEnd();
}

void DibujarFigura(const Figura& f, bool seleccionada) {
    std::set<Celda> celdasBorde, celdasRelleno;
    ObtenerCeldasFigura(f, celdasBorde, celdasRelleno);

    // 1. Dibujar celdas de relleno
    for (const auto& c : celdasRelleno) {
        DibujarCelda(c.first, c.second, f.relleno);
    }

    // 2. Dibujar celdas de borde encima
    for (const auto& c : celdasBorde) {
        DibujarCelda(c.first, c.second, f.borde);
    }

    // 3. Resaltado de selección ajustado exactamente a las celdas ocupadas
    if (seleccionada) {
        int minC = 1e9, maxC = -1e9, minF = 1e9, maxF = -1e9;
        auto expandir = [&](const std::set<Celda>& s) {
            for (const auto& c : s) {
                minC = std::min(minC, c.first);
                maxC = std::max(maxC, c.first);
                minF = std::min(minF, c.second);
                maxF = std::max(maxF, c.second);
            }
            };
        expandir(celdasBorde);
        expandir(celdasRelleno);

        if (minC <= maxC && minF <= maxF) {
            float x0 = minC * tamGrilla;
            float y0 = minF * tamGrilla;
            float x1 = (maxC + 1) * tamGrilla;
            float y1 = (maxF + 1) * tamGrilla;

            glColor3f(1.0f, 0.35f, 0.0f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x0, y0);
            glVertex2f(x1, y0);
            glVertex2f(x1, y1);
            glVertex2f(x0, y1);
            glEnd();
        }
    }
}

void DibujarCursorGrilla(int col, int fila) {
    float x0 = col * tamGrilla;
    float y0 = fila * tamGrilla;
    if (herramientaActual == PINCEL_GRILLA) {
        glColor3fv(colorRelleno);
    }
    else {
        glColor3f(0.10f, 0.50f, 1.0f);
    }
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x0, y0);
    glVertex2f(x0 + tamGrilla, y0);
    glVertex2f(x0 + tamGrilla, y0 + tamGrilla);
    glVertex2f(x0, y0 + tamGrilla);
    glEnd();
}

// interfaz diferente

void AplicarTemaModerno() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f; // Panel acoplado sin bordes redondeados externos
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(10.0f, 6.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.WindowBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.12f, 0.15f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.22f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.17f, 0.19f, 0.24f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.23f, 0.26f, 0.33f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.29f, 0.37f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.48f, 0.88f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.55f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.65f, 1.00f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.25f, 0.60f, 1.00f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.22f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.58f, 0.65f, 1.00f);
}
// --- Interfaz y entrada ---

void DibujarInterfaz(ImGuiIO& io) {
    bool eliminarSel = false;
    bool deshacer = false;
    bool limpiar = false;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::Begin("Herramientas");

    ImGui::Text("Herramienta:");
    if (ImGui::RadioButton("Seleccionar / Mover", herramientaActual == SELECCIONAR))       herramientaActual = SELECCIONAR;
    if (ImGui::RadioButton("Pincel (celdas grilla)", herramientaActual == PINCEL_GRILLA)) herramientaActual = PINCEL_GRILLA;
    if (ImGui::RadioButton("Linea", herramientaActual == LINEA))                          herramientaActual = LINEA;
    if (ImGui::RadioButton("Cuadrado", herramientaActual == CUADRADO))                    herramientaActual = CUADRADO;
    if (ImGui::RadioButton("Circulo", herramientaActual == CIRCULO))                      herramientaActual = CIRCULO;
    if (ImGui::RadioButton("Triangulo", herramientaActual == TRIANGULO))                  herramientaActual = TRIANGULO;
    if (ImGui::RadioButton("Bote de pintura", herramientaActual == BOTEPINTURA))          herramientaActual = BOTEPINTURA;
    if (ImGui::RadioButton("Borrador", herramientaActual == BORRADOR))                    herramientaActual = BORRADOR;

    if (herramientaActual != SELECCIONAR) seleccion = -1;
    bool editSel = (herramientaActual == SELECCIONAR && seleccion >= 0 && seleccion < (int)figuras.size());

    ImGui::Separator();

    if (herramientaActual == BOTEPINTURA || herramientaActual == PINCEL_GRILLA) {
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.3f, 1.0f),
            herramientaActual == PINCEL_GRILLA ? "Pintar en grilla" : "Bote de pintura");
        ImGui::ColorEdit3("Color", colorRelleno);
    }
    else {
        Figura* fs = editSel ? &figuras[seleccion] : nullptr;
        float* pBorde = fs ? fs->borde : colorBorde;
        float* pRelleno = fs ? fs->relleno : colorRelleno;
        bool* pRellena = fs ? &fs->rellena : &rellenarNuevas;
        bool esLinea = fs ? (fs->tipo == LINEA) : (herramientaActual == LINEA);

        if (editSel) ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Editando figura seleccionada");
        else         ImGui::Text("Propiedades nuevas");

        ImGui::ColorEdit3("Color celdas", pBorde);
        if (!esLinea) {
            ImGui::Checkbox("Rellenar interior", pRellena);
            if (*pRellena) ImGui::ColorEdit3("Color relleno", pRelleno);
        }
    }

    if (editSel) {
        Figura* fs = &figuras[seleccion];
        ImGui::Separator();
        ImGui::Text("Transformaciones en grilla");
        ImGui::SliderFloat("Rotacion", &fs->angulo, 0.0f, 360.0f, "%.0f deg");

        if (ImGui::Button("-90")) fs->angulo = Normalizar(fs->angulo - 90.0f);
        ImGui::SameLine();
        if (ImGui::Button("+90")) fs->angulo = Normalizar(fs->angulo + 90.0f);
        ImGui::SameLine();
        if (ImGui::Button("-1 celda")) EscalarEnGrilla(*fs, -1);
        ImGui::SameLine();
        if (ImGui::Button("+1 celda")) EscalarEnGrilla(*fs, 1);

        if (ImGui::Button("Eliminar (Supr)", ImVec2(-1, 0))) eliminarSel = true;
    }

    ImGui::Separator();
    ImGui::Text("Grilla y Lienzo");
    ImGui::ColorEdit3("Fondo", colorFondo);
    ImGui::SliderFloat("Tamano celda", &tamGrilla, 8.0f, 50.0f, "%.0f px");

    ImGui::Spacing();
    if (ImGui::Button("Deshacer", ImVec2(110, 28))) deshacer = true;
    ImGui::SameLine();
    if (ImGui::Button("Limpiar", ImVec2(110, 28))) limpiar = true;

    ImGui::End();

    if (eliminarSel || (editSel && !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Delete, false))) {
        EliminarFigura(seleccion);
    }
    if (deshacer && modo == 0 && !figuras.empty()) {
        figuras.pop_back();
        if (seleccion >= (int)figuras.size()) seleccion = -1;
    }
    if (limpiar) {
        figuras.clear();
        seleccion = -1;
        modo = 0;
    }
}

void ProcesarRaton(ImGuiIO& io) {
    int col = PantallaACol(io.MousePos.x);
    int fila = PantallaAFila(io.MousePos.y);

    // Rueda del ratón: Ctrl+Rueda escala por celdas, Rueda sola rota en pasos de 15°
    if (!io.WantCaptureMouse && seleccion >= 0 && seleccion < (int)figuras.size() && io.MouseWheel != 0.0f) {
        if (io.KeyCtrl) {
            EscalarEnGrilla(figuras[seleccion], io.MouseWheel > 0 ? 1 : -1);
        }
        else {
            figuras[seleccion].angulo = Normalizar(figuras[seleccion].angulo + (io.MouseWheel > 0 ? 15.0f : -15.0f));
        }
    }

    // Clic izquierdo
    if (!io.WantCaptureMouse && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (herramientaActual == SELECCIONAR) {
            seleccion = BuscarFigura(col, fila);
            if (seleccion >= 0) {
                modo = 2;
                offsetCol = figuras[seleccion].cx - col;
                offsetFila = figuras[seleccion].cy - fila;
            }
        }
        else if (herramientaActual == BOTEPINTURA) {
            int i = BuscarFigura(col, fila);
            if (i >= 0 && figuras[i].tipo != LINEA) {
                figuras[i].rellena = true;
                std::copy(colorRelleno, colorRelleno + 3, figuras[i].relleno);
            }
        }
        else if (herramientaActual == PINCEL_GRILLA) {
            PintarCeldaGrilla(col, fila);
        }
        else if (herramientaActual != BORRADOR) {
            modo = 1;
            inicioCol = actualCol = col;
            inicioFila = actualFila = fila;
        }
    }

    // Pintar celdas continuamente
    if (herramientaActual == PINCEL_GRILLA && !io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        PintarCeldaGrilla(col, fila);
    }

    // Borrador continuo
    if (herramientaActual == BORRADOR && !io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        int i = BuscarFigura(col, fila);
        if (i >= 0) EliminarFigura(i);
    }

    // Arrastrar figuras estrictamente por celdas de grilla
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (modo == 1) {
            actualCol = col;
            actualFila = fila;
        }
        else if (modo == 2 && seleccion >= 0 && seleccion < (int)figuras.size()) {
            Figura& f = figuras[seleccion];
            float esqCol = std::round((col + offsetCol) - f.hw);
            float esqFila = std::round((fila + offsetFila) - f.hh);
            f.cx = esqCol + f.hw;
            f.cy = esqFila + f.hh;
        }
    }

    // Soltar
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (modo == 1) {
            Figura nueva;
            if (CrearFigura(herramientaActual, inicioCol, inicioFila, actualCol, actualFila, nueva)) {
                figuras.push_back(nueva);
            }
        }
        modo = 0;
    }
}

int main() {
    glfwSetErrorCallback([](int code, const char* desc) {
        std::cerr << "Error GLFW " << code << ": " << desc << std::endl;
        });

    if (!glfwInit()) return -1;

    GLFWwindow* ventana = glfwCreateWindow(1200, 750, "Paint Pixel/Grilla en C++ con OpenGL", nullptr, nullptr);
    if (!ventana) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(ventana);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwDestroyWindow(ventana);
        glfwTerminate();
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui::StyleColorsDark();
    AplicarTemaModerno();

    ImGui_ImplGlfw_InitForOpenGL(ventana, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    while (!glfwWindowShouldClose(ventana)) {
        glfwPollEvents();
        
        int ancho, alto;
        glfwGetWindowSize(ventana, &ancho, &alto);
        if (ancho <= 0 || alto <= 0) {
            glfwWaitEventsTimeout(0.1);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        DibujarInterfaz(io);
        ProcesarRaton(io);

        ImGui::Render();

        int fbAncho, fbAlto;
        glfwGetFramebufferSize(ventana, &fbAncho, &fbAlto);
        glViewport(0, 0, fbAncho, fbAlto);
        glClearColor(colorFondo[0], colorFondo[1], colorFondo[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, ancho, alto, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // 1. Figuras rasterizadas en celdas
        for (int i = 0; i < (int)figuras.size(); i++) {
            DibujarFigura(figuras[i], i == seleccion);
        }

        // 2. Vista previa mientras arrastras
        if (modo == 1) {
            Figura previa;
            if (CrearFigura(herramientaActual, inicioCol, inicioFila, actualCol, actualFila, previa)) {
                DibujarFigura(previa, false);
            }
        }

        // 3. Grilla dibujada encima para que se vean las divisiones de cada celda coloreada (como en tu captura)
        DibujarGrilla(ancho, alto);

        // 4. Cursor de celda bajo el ratón
        if (!io.WantCaptureMouse) {
            DibujarCursorGrilla(PantallaACol(io.MousePos.x), PantallaAFila(io.MousePos.y));
        }

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(ventana);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(ventana);
    glfwTerminate();
    return 0;
}