/****************************************************************************
** Copyright (c) 2018, Fougue Ltd. <http://www.fougue.pro>
** All rights reserved.
** See license at https://github.com/fougue/mayo/blob/master/LICENSE.txt
****************************************************************************/

#include "gpx_mesh_item.h"

#include "options.h"
#include "fougtools/occtools/qt_utils.h"

#include <AIS_InteractiveContext.hxx>
#include <MeshVS_DrawerAttribute.hxx>
#include <MeshVS_Drawer.hxx>
#include <MeshVS_Mesh.hxx>
#include <MeshVS_MeshPrsBuilder.hxx>
#include <XSDRAWSTLVRML_DataSource.hxx>

namespace Mayo {

namespace Internal {

static void redisplayAndUpdateViewer(AIS_InteractiveObject* ptrGpx)
{
    ptrGpx->Redisplay(Standard_True); // All modes
    ptrGpx->GetContext()->UpdateCurrentViewer();
}

} // namespace Internal

GpxMeshItem::GpxMeshItem(MeshItem *item)
    : GpxCovariantDocumentItem(item),
      propertyDisplayMode(this, tr("Display mode"), &enum_DisplayMode()),
      propertyShowEdges(this, tr("Show edges")),
      propertyShowNodes(this, tr("Show nodes"))
{
    // Create the MeshVS_Mesh object
    const Options* opts = Options::instance();
    Handle_XSDRAWSTLVRML_DataSource dataSource =
            new XSDRAWSTLVRML_DataSource(item->triangulation());
    Handle_MeshVS_Mesh meshVisu = new MeshVS_Mesh;
    meshVisu->SetDataSource(dataSource);
    // meshVisu->AddBuilder(..., Standard_False); -> No selection
    meshVisu->AddBuilder(new MeshVS_MeshPrsBuilder(meshVisu), Standard_True);
    // -- MeshVS_DrawerAttribute
    meshVisu->GetDrawer()->SetBoolean(
                MeshVS_DA_ShowEdges, opts->meshDefaultShowEdges());
    meshVisu->GetDrawer()->SetBoolean(
                MeshVS_DA_DisplayNodes, opts->meshDefaultShowNodes());
    meshVisu->GetDrawer()->SetMaterial(
                MeshVS_DA_FrontMaterial,
                Graphic3d_MaterialAspect(opts->meshDefaultMaterial()));
    meshVisu->GetDrawer()->SetColor(
                MeshVS_DA_InteriorColor,
                occ::QtUtils::toOccColor(opts->meshDefaultColor()));
    meshVisu->SetDisplayMode(MeshVS_DMF_Shading);
    // -- Wireframe as default hilight mode
    meshVisu->SetHilightMode(MeshVS_DMF_WireFrame);
    meshVisu->SetMeshSelMethod(MeshVS_MSM_PRECISE);

    m_hndGpxObject = meshVisu;

    // Init properties
    Mayo_PropertyChangedBlocker(this);
    // -- Material
    Graphic3d_MaterialAspect materialAspect;
    meshVisu->GetDrawer()->GetMaterial(
                MeshVS_DA_FrontMaterial, materialAspect);
    this->propertyMaterial.setValue(materialAspect.Name());
    // -- Color
    Quantity_Color color;
    meshVisu->GetDrawer()->GetColor(MeshVS_DA_InteriorColor, color);
    this->propertyColor.setValue(color);
    // -- Display mode
    this->propertyDisplayMode.setValue(meshVisu->DisplayMode());
    // -- Show edges
    Standard_Boolean boolVal;
    meshVisu->GetDrawer()->GetBoolean(MeshVS_DA_ShowEdges, boolVal);
    this->propertyShowEdges.setValue(boolVal == Standard_True);
    // -- Show nodes
    meshVisu->GetDrawer()->GetBoolean(MeshVS_DA_DisplayNodes, boolVal);
    this->propertyShowNodes.setValue(boolVal == Standard_True);
}

void GpxMeshItem::onPropertyChanged(Property *prop)
{
    Handle_AIS_InteractiveContext cxt = this->gpxObject()->GetContext();
    Handle_AIS_InteractiveObject hndGpx = this->handleGpxObject();
    MeshVS_Mesh* ptrGpx = this->gpxObject();
    if (prop == &this->propertyMaterial) {
        const Graphic3d_NameOfMaterial mat =
                this->propertyMaterial.valueAs<Graphic3d_NameOfMaterial>();
        ptrGpx->GetDrawer()->SetMaterial(
                    MeshVS_DA_FrontMaterial, Graphic3d_MaterialAspect(mat));
        Internal::redisplayAndUpdateViewer(ptrGpx);
    }
    else if (prop == &this->propertyColor) {
        ptrGpx->GetDrawer()->SetColor(
                    MeshVS_DA_InteriorColor, this->propertyColor.value());
        Internal::redisplayAndUpdateViewer(ptrGpx);
    }
    else if (prop == &this->propertyDisplayMode) {
        cxt->SetDisplayMode(
                    hndGpx, this->propertyDisplayMode.value(), Standard_True);
        //ptrGpx->SetDisplayMode(this->propertyDisplayMode.value());
    }
    else if (prop == &this->propertyShowEdges) {
        ptrGpx->GetDrawer()->SetBoolean(
                    MeshVS_DA_ShowEdges, this->propertyShowEdges.value());
        Internal::redisplayAndUpdateViewer(ptrGpx);
    }
    else if (prop == &this->propertyShowNodes) {
        ptrGpx->GetDrawer()->SetBoolean(
                    MeshVS_DA_DisplayNodes, this->propertyShowNodes.value());
        Internal::redisplayAndUpdateViewer(ptrGpx);
    }
    GpxDocumentItem::onPropertyChanged(prop);
}

const Enumeration &GpxMeshItem::enum_DisplayMode()
{
    static Enumeration enumeration;
    if (enumeration.size() == 0) {
        enumeration.map(MeshVS_DMF_WireFrame, tr("Wireframe"));
        enumeration.map(MeshVS_DMF_Shading, tr("Shaded"));
        enumeration.map(MeshVS_DMF_Shrink, tr("Shrink"));
    }
    return enumeration;
}

} // namespace Mayo
