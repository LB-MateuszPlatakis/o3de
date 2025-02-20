/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>


#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyDoubleSpinCtrl.hxx>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <Editor/ObjectEditor.h>
#include <Editor/ColliderHelpers.h>
#include <Editor/ColliderContainerWidget.h>

#include <Tests/UI/SkeletonOutlinerTestFixture.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/SimpleActors.h>

#include <QApplication>
#include <QTest>

namespace EMotionFX
{
    void SkeletonOutlinerTestFixture::SetUpPhysics()
    {
        EMStudio::GetMainWindow()->ApplicationModeChanged("Physics");
        const int numJoints = 6;
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, numJoints, "TestsActor");

        m_actor = actorAsset->GetActor();
        CreateSkeletonAndModelIndices();
        EXPECT_EQ(m_indexList.size(), numJoints);
    }

    void SkeletonOutlinerTestFixture::AddColliderViaAddComponentButton(QString label, QString subLevelLabel)
    {
        EXPECT_GT(m_indexList.size(), 3) << "Make sure to have a skeleton";
        // Find the 3rd joint after the RootJoint in the TreeView and select it
        SelectIndexes(m_indexList, m_treeView, 3, 3);

        auto* treeView = GetAddCollidersTreeView();
        auto* model = treeView->model();

        //  find indices
        QModelIndexList indices = model->match(
            model->index(0, 0),
            Qt::DisplayRole,
            QVariant::fromValue(label),
                    -1,
            Qt::MatchRecursive);

        if (subLevelLabel != "")
        {
            indices = model->match(
                model->index(0, 0, indices[0]),
                Qt::DisplayRole,
                QVariant::fromValue(subLevelLabel),
                    -1,
                Qt::MatchRecursive);
        }
        //  check indices
        EXPECT_GE(indices.size(), 1) << "Label not found";
        EXPECT_LE(indices.size(), 1) << "Label is not unique";

        //  click first index
        auto index = indices[0];
        treeView->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->clicked(index);
    }

    void SkeletonOutlinerTestFixture::ShowJointPropertyWidget()
    {
        auto* mainwindow = new QMainWindow;

        auto* widget = GetJointPropertyWidget();
        auto* mainWidget = new QWidget;
        auto* mainLayout = new QVBoxLayout;
        mainLayout->addWidget(widget);
        mainLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));
        mainWidget->setLayout(mainLayout);

        mainwindow->setMinimumHeight(500);
        mainwindow->setCentralWidget(mainWidget);
        mainwindow->show();

        QApplication::processEvents();
    }

    //
    //  Test Cases
    //
    TEST_F(SkeletonOutlinerTestFixture, AddClothCollider)
    {
        SetUpPhysics();

        // Find the 3rd joint after the RootJoint in the TreeView and select it
        SelectIndexes(m_indexList, m_treeView, 3, 3);

        AddColliderViaAddComponentButton("Add Cloth Collider", "Sphere");

        ShowJointPropertyWidget();

        EXPECT_TRUE(ColliderHelpers::NodeHasClothCollider(m_indexList[3]));
    }

    TEST_F(SkeletonOutlinerTestFixture, ChangeClothColliderValue)
    {
        SetUpPhysics();

        AddColliderViaAddComponentButton("Add Cloth Collider", "Capsule");

        // Check the node is in the ragdoll
        EXPECT_TRUE(ColliderHelpers::NodeHasClothCollider(m_indexList[3]));

        // Get the widget
        auto* widget = GetJointPropertyWidget();

        // Get a value widget
        auto propertyEditor = widget->findChild<AzToolsFramework::ReflectedPropertyEditor*>("PropertyEditor");

        // Get list of all PropertyRowWidgets (and their InstanceDataNodes)
        const auto list = propertyEditor->GetWidgets();
        ASSERT_GT(list.size(), 0) << "Did not find any PropertyRowWidgets";

        // Look for PropertyRowWidget for "Name"
        AzToolsFramework::PropertyRowWidget* propertyRow = nullptr;
        for (const auto& item : list)
        {
            if (item.second->objectName() == "Height")
            {
                propertyRow = item.second;
            }
        }

        // Change it
        auto *lineEdit = static_cast<AzToolsFramework::PropertyDoubleSpinCtrl*>(propertyRow->GetChildWidget());
        ASSERT_TRUE(lineEdit) << "Did not find Editing handle";
        lineEdit->setValue(3.89);
        lineEdit->editingFinished();

        // Make sure propertyWidget are created correctly
        ShowJointPropertyWidget();
        // We did not crash, at least
    }

    TEST_F(SkeletonOutlinerTestFixture, DISABLED_CopyAndPaste)
    {
        SetUpPhysics();

        // create a cloth collider to copy it
        AddColliderViaAddComponentButton("Add Cloth Collider", "Capsule");

        // copy it
        auto* jointWidget = GetJointPropertyWidget()->findChild<ClothJointWidget*>();
        auto* colliderContainerWidget = jointWidget->findChild<ColliderContainerWidget*>();
        emit colliderContainerWidget->CopyCollider(0);

        AddColliderViaAddComponentButton("Paste as Hit Detection Collider");
        ShowJointPropertyWidget();
       EXPECT_EQ(0, 1) << "This test is not implemented";
    }
} // namespace EMotionFX
