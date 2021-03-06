/****************************************************************************
** Copyright (c) 2018, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "gui_document.h"

#include "application_item.h"
#include "bnd_utils.h"
#include "brep_utils.h"
#include "document.h"
#include "document_item.h"
#include "gpx_document_item.h"
#include "gpx_mesh_item.h"
#include "gpx_utils.h"
#include "gpx_xde_document_item.h"
#include "mesh_item.h"
#include "xde_document_item.h"

#include <AIS_Trihedron.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Geom_Axis2Placement.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <StdSelect_BRepOwner.hxx>

#include <cassert>

namespace Mayo {

namespace Internal {

template<typename ITEM, typename GPX_ITEM>
bool createGpxIfItemOfType(GpxDocumentItem** gpx, DocumentItem* item)
{
    if (*gpx == nullptr && sameType<ITEM>(item)) {
        *gpx = new GPX_ITEM(static_cast<ITEM*>(item));
        return true;
    }
    return false;
}

static GpxDocumentItem* createGpxForItem(DocumentItem* item)
{
    GpxDocumentItem* gpx = nullptr;
    createGpxIfItemOfType<XdeDocumentItem, GpxXdeDocumentItem>(&gpx, item);
    createGpxIfItemOfType<MeshItem, GpxMeshItem>(&gpx, item);
    return gpx;
}

static Handle_V3d_Viewer createOccViewer()
{
    Handle_Aspect_DisplayConnection dispConnection;
#if (!defined(Q_OS_WIN32) && (!defined(Q_OS_MAC) || defined(MACOSX_USE_GLX)))
    dispConnection = new Aspect_DisplayConnection(std::getenv("DISPLAY"));
#endif
    Handle_Graphic3d_GraphicDriver gpxDriver = new OpenGl_GraphicDriver(dispConnection);
    Handle_V3d_Viewer viewer = new V3d_Viewer(gpxDriver);
    viewer->SetDefaultViewSize(1000.);
    viewer->SetDefaultViewProj(V3d_XposYnegZpos);
    viewer->SetComputedMode(Standard_True);
    viewer->SetDefaultComputedMode(Standard_True);
//    viewer->SetDefaultVisualization(V3d_ZBUFFER);
//    viewer->SetDefaultShadingModel(V3d_GOURAUD);
    viewer->SetDefaultLights();
    viewer->SetLightOn();

    return viewer;
}

static Handle_AIS_Trihedron createOriginTrihedron()
{
    Handle_Geom_Axis2Placement axis = new Geom_Axis2Placement(gp::XOY());
    Handle_AIS_Trihedron aisTrihedron = new AIS_Trihedron(axis);
    aisTrihedron->SetDatumDisplayMode(Prs3d_DM_Shaded);
    aisTrihedron->SetDatumPartColor(Prs3d_DP_XArrow, Quantity_NOC_RED2);
    aisTrihedron->SetDatumPartColor(Prs3d_DP_YArrow, Quantity_NOC_GREEN2);
    aisTrihedron->SetDatumPartColor(Prs3d_DP_ZArrow, Quantity_NOC_BLUE2);
    aisTrihedron->SetDatumPartColor(Prs3d_DP_XAxis, Quantity_NOC_RED2);
    aisTrihedron->SetDatumPartColor(Prs3d_DP_YAxis, Quantity_NOC_GREEN2);
    aisTrihedron->SetDatumPartColor(Prs3d_DP_ZAxis, Quantity_NOC_BLUE2);
    aisTrihedron->SetLabel(Prs3d_DP_XAxis, "");
    aisTrihedron->SetLabel(Prs3d_DP_YAxis, "");
    aisTrihedron->SetLabel(Prs3d_DP_ZAxis, "");
    //aisTrihedron->SetTextColor(Quantity_NOC_GRAY40);
    aisTrihedron->SetSize(60);
    Handle_Graphic3d_TransformPers trsf =
            new Graphic3d_TransformPers(Graphic3d_TMF_ZoomPers, axis->Ax2().Location());
    aisTrihedron->SetTransformPersistence(trsf);
    aisTrihedron->SetInfiniteState(true);
    return aisTrihedron;
}

} // namespace Internal

GuiDocument::GuiDocument(Document *doc)
    : m_document(doc),
      m_v3dViewer(Internal::createOccViewer()),
      m_aisContext(new AIS_InteractiveContext(m_v3dViewer)),
      m_v3dView(m_v3dViewer->CreateView())
{
    assert(doc != nullptr);

    // 3D view - Enable anti-aliasing with MSAA
    m_v3dView->ChangeRenderingParams().IsAntialiasingEnabled = true;
    m_v3dView->ChangeRenderingParams().NbMsaaSamples = 4;
    // 3D view - Set gradient background
    m_v3dView->SetBgGradientColors(
                Quantity_Color(0.5, 0.58, 1., Quantity_TOC_RGB),
                Quantity_NOC_WHITE,
                Aspect_GFM_VER);
    // 3D view - Add shaded trihedron located in the bottom-left corner
    m_v3dView->TriedronDisplay(
                Aspect_TOTP_LEFT_LOWER,
                Quantity_NOC_GRAY50,
                0.075,
                V3d_ZBUFFER);
    // 3D scene - Add trihedron placed at the origin
    m_aisContext->Display(Internal::createOriginTrihedron(), true);

    QObject::connect(doc, &Document::itemAdded, this, &GuiDocument::onItemAdded);
    QObject::connect(doc, &Document::itemErased, this, &GuiDocument::onItemErased);
}

Document *GuiDocument::document() const
{
    return m_document;
}

const Handle_V3d_View &GuiDocument::v3dView() const
{
    return m_v3dView;
}

const Handle_AIS_InteractiveContext &GuiDocument::aisInteractiveContext() const
{
    return m_aisContext;
}

GpxDocumentItem *GuiDocument::findItemGpx(const DocumentItem *item) const
{
    const GuiDocumentItem* guiDocItem = this->findGuiDocumentItem(item);
    return guiDocItem != nullptr ? guiDocItem->gpxDocItem : nullptr;
}

const Bnd_Box &GuiDocument::gpxBoundingBox() const
{
    return m_gpxBoundingBox;
}

void GuiDocument::toggleItemSelected(const ApplicationItem &appItem)
{
    if (appItem.document() != this->document())
        return;
    if (appItem.isXdeAssemblyNode()) {
        const XdeAssemblyNode& xdeAsmNode = appItem.xdeAssemblyNode();
        const XdeDocumentItem* xdeItem = xdeAsmNode.ownerDocItem;
        const GuiDocumentItem* guiItem = this->findGuiDocumentItem(xdeItem);
        if (guiItem != nullptr) {
            const TopLoc_Location shapeLoc =
                    xdeItem->shapeAbsoluteLocation(xdeAsmNode.nodeId);
            const TopoDS_Shape shape =
                    xdeItem->shape(xdeAsmNode.label()).Located(shapeLoc);
            std::vector<TopoDS_Face> vecFace;
            if (BRepUtils::moreComplex(shape.ShapeType(), TopAbs_FACE)) {
                BRepUtils::forEachSubFace(shape, [&](const TopoDS_Face& face) {
                    vecFace.push_back(face);
                });
            }
            else if (shape.ShapeType() == TopAbs_FACE) {
                vecFace.push_back(TopoDS::Face(shape));
            }
            for (const TopoDS_Face& face : vecFace) {
                auto brepOwner = guiItem->findBrepOwner(face);
                if (!brepOwner.IsNull())
                    m_aisContext->AddOrRemoveSelected(brepOwner, false);
            }
        }
    }
    else if (appItem.isDocumentItem()) {
        const GpxDocumentItem* gpxItem = this->findItemGpx(appItem.documentItem());
        if (gpxItem != nullptr)
            m_aisContext->AddOrRemoveSelected(gpxItem->handleGpxObject(), false);
    }
    else if (appItem.isDocument()) {
        for (const DocumentItem* docItem : appItem.document()->rootItems()) {
            const GpxDocumentItem* gpxItem = this->findItemGpx(docItem);
            if (gpxItem != nullptr)
                m_aisContext->AddOrRemoveSelected(gpxItem->handleGpxObject(), false);
        }
    }
}

void GuiDocument::clearItemSelection()
{
    m_aisContext->ClearSelected(false);
}

void GuiDocument::updateV3dViewer()
{
    m_aisContext->UpdateCurrentViewer();
}

void GuiDocument::onItemAdded(DocumentItem *item)
{
    GuiDocumentItem guiItem(item, Internal::createGpxForItem(item));
    const Handle_AIS_InteractiveObject aisObject = guiItem.gpxDocItem->handleGpxObject();
    m_aisContext->Display(aisObject, true);
    if (sameType<XdeDocumentItem>(item)) {
        m_aisContext->Activate(aisObject, AIS_Shape::SelectionMode(TopAbs_VERTEX));
        m_aisContext->Activate(aisObject, AIS_Shape::SelectionMode(TopAbs_EDGE));
        m_aisContext->Activate(aisObject, AIS_Shape::SelectionMode(TopAbs_FACE));
//        m_aisContext->Activate(aisObject, AIS_Shape::SelectionMode(TopAbs_SOLID));
        opencascade::handle<SelectMgr_IndexedMapOfOwner> mapEntityOwner;
        m_aisContext->EntityOwners(
                    mapEntityOwner, aisObject, AIS_Shape::SelectionMode(TopAbs_FACE));
        guiItem.vecGpxEntityOwner.reserve(mapEntityOwner->Extent());
        for (auto it = mapEntityOwner->cbegin(); it != mapEntityOwner->cend(); ++it)
            guiItem.vecGpxEntityOwner.push_back(std::move(*it));
    }
    m_vecGuiDocumentItem.emplace_back(std::move(guiItem));
    GpxUtils::V3dView_fitAll(m_v3dView);
    BndUtils::add(&m_gpxBoundingBox, BndUtils::get(aisObject));
    emit gpxBoundingBoxChanged(m_gpxBoundingBox);
}

void GuiDocument::onItemErased(const DocumentItem *item)
{
    auto itFound = std::find_if(
                m_vecGuiDocumentItem.begin(),
                m_vecGuiDocumentItem.end(),
                [=](const GuiDocumentItem& guiItem) { return guiItem.docItem == item; });
    if (itFound != m_vecGuiDocumentItem.end()) {
        // Delete gpx item
        GpxDocumentItem* gpxDocItem = itFound->gpxDocItem;
        GpxUtils::AisContext_eraseObject(m_aisContext, gpxDocItem->handleGpxObject());
        delete gpxDocItem;
        m_vecGuiDocumentItem.erase(itFound);

        // Recompute bounding box
        m_gpxBoundingBox.SetVoid();
        for (const GuiDocumentItem& guiItem : m_vecGuiDocumentItem) {
            const Bnd_Box otherBox = BndUtils::get(guiItem.gpxDocItem->handleGpxObject());
            BndUtils::add(&m_gpxBoundingBox, otherBox);
        }
        emit gpxBoundingBoxChanged(m_gpxBoundingBox);
    }
}

const GuiDocument::GuiDocumentItem*
GuiDocument::findGuiDocumentItem(const DocumentItem *item) const
{
    auto itFound = std::find_if(
                m_vecGuiDocumentItem.cbegin(),
                m_vecGuiDocumentItem.cend(),
                [=](const GuiDocumentItem& guiItem) { return guiItem.docItem == item; });
    return itFound != m_vecGuiDocumentItem.cend() ? &(*itFound) : nullptr;
}

GuiDocument::GuiDocumentItem::GuiDocumentItem(DocumentItem *item, GpxDocumentItem *gpx)
    : docItem(item), gpxDocItem(gpx)
{
}

Handle_SelectMgr_EntityOwner GuiDocument::GuiDocumentItem::findBrepOwner(const TopoDS_Face& face) const
{
    auto itFound = std::find_if(
                vecGpxEntityOwner.cbegin(),
                vecGpxEntityOwner.cend(),
                [=](const Handle_SelectMgr_EntityOwner& owner) {
        auto brepOwner = Handle_StdSelect_BRepOwner::DownCast(owner);
        return !brepOwner.IsNull() ? brepOwner->Shape() == face : false;
    });
    return itFound != vecGpxEntityOwner.cend() ?
                *itFound : Handle_SelectMgr_EntityOwner();
}

} // namespace Mayo
