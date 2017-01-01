/*
Copyright(c) 2016-2017 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES =====================
#include <QWidget>
#include "Core/GameObject.h"
#include "DirectusTransform.h"
#include "DirectusCamera.h"
#include "DirectusMeshRenderer.h"
#include "DirectusMaterial.h"
#include "DirectusCollider.h"
#include "DirectusRigidBody.h"
#include "DirectusLight.h"
#include "DirectusScript.h"
#include "DirectusMeshFilter.h"
#include "DirectusMeshCollider.h"
#include "DirectusCore.h"
#include "DirectusAudioSource.h"
#include "DirectusAudioListener.h"
#include "DirectusIComponent.h"
//================================

class DirectusInspector : public QWidget
{
    Q_OBJECT
public:
    explicit DirectusInspector(QWidget *parent = 0);
    void SetDirectusCore(DirectusCore* directusCore);
    void Initialize(QWidget* mainWindow);
    GameObject* GetInspectedGameObject();

    void Clear();
    void InspectMaterialFile(const std::string &filepath);
    Socket* GetSocket();
    DirectusMaterial* GetMaterialComponent();

    //= DROP ===========================================
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragMoveEvent (QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);

protected:
    virtual void paintEvent(QPaintEvent* evt);

private:
    std::vector<Script*> FitScriptVectorToGameObject();

    DirectusIComponent* m_transform;
    DirectusIComponent* m_camera;
    DirectusIComponent* m_meshRenderer;
    DirectusIComponent* m_material;
    DirectusIComponent* m_collider;
    DirectusIComponent* m_rigidBody;
    DirectusIComponent* m_light;
    std::vector<DirectusIComponent*> m_scripts;
    DirectusIComponent* m_meshFilter;
    DirectusIComponent* m_meshCollider;
    DirectusIComponent* m_audioSource;
    DirectusIComponent* m_audioListener;

    DirectusCore* m_directusCore;
    GameObject* m_inspectedGameObject;
    QWidget* m_mainWindow;
    bool m_initialized;

public slots:
    void Inspect(GameObject* gameobject);
};
