#pragma once

template <typename T>
class CSingleTone {
protected:
    CSingleTone() {}
    ~CSingleTone() {}
public:
    static T* getInstance() {
        static T m_instance;
        return &m_instance;
    }
};
