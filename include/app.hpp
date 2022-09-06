#ifndef APP_H
#define APP_H

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
};

#endif // APP_H
