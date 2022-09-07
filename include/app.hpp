#ifndef APP_H
#define APP_H

#include <memory>
#include <openfolderwidget.h>
#include <openimagewidget.h>
#include <string>
#include <vector>

class App
{
public:
    App(const std::vector<std::string> &args);
    virtual ~App();

    bool Init();
    int Run();

    void OnInit();
    void OnFrame();
    void OnResize(int width, int height);
    void OnExit();

    template <class T>
    T *GetWindowHandle() const;

protected:
    const std::vector<std::string> &_args;
    int _width = 1024;
    int _height = 768;

    template <class T>
    void SetWindowHandle(T *handle);

    void ClearWindowHandle();

private:
    void *_windowHandle;

    std::vector<std::unique_ptr<OpenFolderWidget>> _openFolders;
    std::vector<std::unique_ptr<OpenImageWidget>> _openImages;

    void ActivatePath(
        const std::filesystem::path &path);
};

#endif // APP_H
