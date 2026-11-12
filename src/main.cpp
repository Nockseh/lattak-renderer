#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#if defined(__EMSCRIPTEN__)
  #include <emscripten/emscripten.h>
#endif

// Types //
class Panel {
public:
    Rectangle rect = {0,0,0,0};
    Color background = Color{255,0,255,255};
    const char* title;
};

// Config //
const int WindowWidth = 1280;
const int WindowHeight = 720;
const int ViewportWidth = 2 * WindowWidth / 3;
const int ViewportHeight = WindowHeight;
const float CellSizeX = 1.0f;
const float CellSizeZ = 1.0f;
const Vector3 BoardPosition = {0.0f, 0.0f, 0.0f};
const Color BaseColor1 = Color{211, 192, 178, 255};
const Color BaseColor2 = Color{147, 128, 116, 255};
const Color BaseColor3 = Color{94, 66, 76, 255};
const Color BaseColor4 = Color{44, 26, 28, 255};
const Color AccentColor1 = Color{121, 173, 200, 255};
const Color AccentColor2 = Color{49, 95, 122, 255};
const Color AccentColor3 = Color{49, 65, 96, 255};
const Color AccentColor4 = Color{33, 32, 68, 255};

Panel infoPanel = {
    .rect = {ViewportWidth, 0, WindowWidth-ViewportWidth, WindowHeight},
    .background = BaseColor3,
    .title = "info panel"
};


// Globals //
static Camera3D camera;
static Model boardFrameModel;
static RenderTexture2D viewport;
static Font mainFont24;

static int boardRows = 5;
static int boardCols = 5;

static float camYaw = 0.0f;
static float camPitch = 0.5f;
static float camDist = std::max(boardRows, boardCols);

// Definitions //
int LoadAssets();
int Initialize();
void Update();

// Main //
int main() {
    InitWindow(WindowWidth, WindowHeight, "WindowTitle");

    #if !defined(__EMSCRIPTEN__)
        ChangeDirectory(GetApplicationDirectory());
    #endif
    LoadAssets();

    Initialize();

    //loops
    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(Update, 0, 1); //intentionally throws
    #else
        SetTargetFPS(120);
        while (!WindowShouldClose()) {
            Update();
        }
    #endif

    //cleanup only reached by non-web environments
    CloseWindow();
    return 0;
}

int LoadAssets() {
    mainFont24 = LoadFontEx("assets/fonts/atkinson-hyperlegible-mono.ttf", 24, nullptr, 0);

    boardFrameModel = LoadModel("assets/models/board-frame.obj");
    Texture2D boardFrameTexture = LoadTexture("assets/textures/wood-cherry-lacquered.png");
    boardFrameModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = boardFrameTexture;

    return 0;
}

int Initialize() {
    camera = {
        .position = (Vector3){0.0f, 0.0f, 0.0f},
        .target = (Vector3){0.0f, 0.0f, 0.0f},
        .up = (Vector3){0.0f, 1.0f, 0.0f},
        .fovy = 50.0f,
        .projection = CAMERA_PERSPECTIVE
    };

    viewport = LoadRenderTexture(ViewportWidth, ViewportHeight);

    return 0;
}

void Update() {
    float dt = GetFrameTime();
    float fps = static_cast<float>(GetFPS());

//input
    Vector2 mousePos = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 d = GetMouseDelta();
        d *= dt * fps;
        camYaw -= d.x * 0.005f;
        camPitch = Clamp(camPitch + d.y * 0.005f, -1.4f, 1.4f);
    }
    float wheelDelta = GetMouseWheelMove(); //positive = up

//physics
    camDist = fmax(camDist -= wheelDelta, 1);
    camera.position = {
        camera.target.x + camDist * cosf(camPitch) * sinf(camYaw),
        camera.target.y + camDist * sinf(camPitch),
        camera.target.z + camDist * cosf(camPitch) * cosf(camYaw),
    };

    Ray cursorRay = GetScreenToWorldRayEx(mousePos, camera, ViewportWidth, ViewportHeight);
    float hitDist = (BoardPosition.y - cursorRay.position.y) / cursorRay.direction.y;
    Vector3 cursorWorldPos = cursorRay.position + cursorRay.direction * hitDist;

//render (3d)
    BeginTextureMode(viewport);
        ClearBackground(AccentColor4);
        BeginMode3D(camera);
        // DrawGrid(10, 10.0f);

        //board
        for (int row = 0; row < boardRows; row++) {
            for (int column = 0; column < boardCols; column++) {
                float cellX = column - boardCols/2.0f + (CellSizeX/2) + BoardPosition.x; //center
                float cellZ = row - boardRows/2.0f + (CellSizeZ/2) + BoardPosition.z;    //center
                bool hovered = false;
                bool cursorWithinX = cursorWorldPos.x >= cellX - CellSizeX/2 && cursorWorldPos.x <= cellX + CellSizeX/2;
                bool cursorWithinZ = cursorWorldPos.z >= cellZ - CellSizeZ/2 && cursorWorldPos.z <= cellZ + CellSizeZ/2;
                if (cursorWithinX && cursorWithinZ) {
                    hovered = true;
                }

                Color cellColor;
                if ((row+column) % 2 == 0) {
                    cellColor = hovered ? BaseColor3 : BaseColor4;
                } else {
                    cellColor = hovered ? BaseColor1 : BaseColor2;
                }

                DrawPlane({cellX, BoardPosition.y, cellZ}, {CellSizeX,CellSizeZ}, cellColor);
            }
        }
        float sizeX = boardCols * CellSizeX;
        float sizeZ = boardRows * CellSizeZ;
        float sizeY = (boardRows + boardCols)/2.0f;
        DrawModelEx(boardFrameModel, BoardPosition, {0,1,0}, 0.0f, {sizeX,sizeY,sizeZ}, WHITE);

        //pieces

        EndMode3D();
    EndTextureMode();

//draw
    BeginDrawing();
        //viewport
        ClearBackground(AccentColor4);
        Rectangle viewRect = {0,0,ViewportWidth, -ViewportHeight}; //OpenGL textures are upside down
        DrawTextureRec(viewport.texture, viewRect, (Vector2){0,0}, WHITE);
        //ui
        DrawRectangleRec(infoPanel.rect, infoPanel.background);
        Vector2 titleSize = MeasureTextEx(mainFont24, infoPanel.title, 24, 0);
        DrawTextEx(mainFont24, infoPanel.title, {(infoPanel.rect.x + infoPanel.rect.width/2) - titleSize.x/2, 0}, 24, 0, BaseColor1);

    EndDrawing();
}