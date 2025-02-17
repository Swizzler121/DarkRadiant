#include "Prefab.h"

#include "i18n.h"
#include "iselection.h"
#include "ipatch.h"
#include "igrid.h"
#include "itextstream.h"
#include "iorthoview.h"
#include "ishaderclipboard.h"

#include "scenelib.h"
#include "shaderlib.h"
#include "patch/Patch.h"

#include "selection/algorithm/General.h"
#include "selectionlib.h"
#include "command/ExecutionFailure.h"

#include "string/case_conv.h"

namespace patch
{

namespace algorithm
{

namespace
{
	// Gets the active/selected shader or the default fallback value
	inline std::string getSelectedShader()
	{
		auto selectedShader = GlobalShaderClipboard().getShaderName();

		if (selectedShader.empty())
		{
			selectedShader = texdef_name_default();
		}

		return selectedShader;
	}
}

void constructPrefab(const AABB& aabb, const std::string& shader, EPatchPrefab eType, 
					 EViewType viewType, std::size_t width, std::size_t height)
{
	GlobalSelectionSystem().setSelectedAll(false);

	scene::INodePtr node(GlobalPatchModule().createPatch(patch::PatchDefType::Def2));
	GlobalMapModule().findOrInsertWorldspawn()->addChildNode(node);

	Patch* patch = Node_getPatch(node);
	patch->setShader(shader);

	patch->ConstructPrefab(aabb, eType, viewType, width, height);
	patch->controlPointsChanged();

	Node_setSelected(node, true);
}

AABB getDefaultBoundsFromSelection()
{
	AABB aabb = GlobalSelectionSystem().getWorkZone().bounds;

	float gridSize = GlobalGrid().getGridSize();

	if (aabb.extents[0] == 0)
	{
		aabb.extents[0] = gridSize;
	}

	if (aabb.extents[1] == 0)
	{
		aabb.extents[1] = gridSize;
	}

	if (aabb.extents[2] == 0)
	{
		aabb.extents[2] = gridSize;
	}

	if (aabb.isValid())
	{
		return aabb;
	}

	return AABB(Vector3(0, 0, 0), Vector3(64, 64, 64));
}

void createPrefabInternal(EPatchPrefab prefabType, const std::string& undoCmdName)
{
	UndoableCommand undo(undoCmdName);

	constructPrefab(getDefaultBoundsFromSelection(), 
					getSelectedShader(),
					prefabType, 
					GlobalXYWndManager().getActiveViewType());
}

void createPrefab(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rError() << "Usage: createPatchPrefab <type>" << std::endl
			<< " with <type> being one of the following: " << std::endl
			<< "cylinder, densecylinder, verydensecylinder, squarecylinder," << std::endl
			<< "sphere, endcap, bevel, cone" << std::endl;
		return;
	}

	std::string typeStr = string::to_lower_copy(args[0].getString());

	if (typeStr == "cylinder")
	{
		createPrefabInternal(eCylinder, "patchCreateCylinder");
	}
	else if (typeStr == "densecylinder")
	{
		createPrefabInternal(eDenseCylinder, "patchCreateDenseCylinder");
	}
	else if (typeStr == "verydensecylinder")
	{
		createPrefabInternal(eVeryDenseCylinder, "patchCreateVeryDenseCylinder");
	}
	else if (typeStr == "squarecylinder")
	{
		createPrefabInternal(eSqCylinder, "patchCreateSquareCylinder");
	}
	else if (typeStr == "sphere")
	{
		createPrefabInternal(eSphere, "patchCreateSphere");
	}
	else if (typeStr == "endcap")
	{
		createPrefabInternal(eEndCap, "patchCreateCaps");
	}
	else if (typeStr == "bevel")
	{
		createPrefabInternal(eBevel, "patchCreateBevel");
	}
	else if (typeStr == "cone")
	{
		createPrefabInternal(eCone, "patchCreateCone");
	}
}

void createCylinder(const cmd::ArgumentList& args)
{
	createPrefabInternal(eCylinder, "patchCreateCylinder");
}

void createDenseCylinder(const cmd::ArgumentList& args)
{
	createPrefabInternal(eDenseCylinder, "patchCreateDenseCylinder");
}

void createVeryDenseCylinder(const cmd::ArgumentList& args)
{
	createPrefabInternal(eVeryDenseCylinder, "patchCreateVeryDenseCylinder");
}

void createSquareCylinder(const cmd::ArgumentList& args)
{
	createPrefabInternal(eSqCylinder, "patchCreateSquareCylinder");
}

void createSphere(const cmd::ArgumentList& args)
{
	createPrefabInternal(eSphere, "patchCreateSphere");
}

void createEndcap(const cmd::ArgumentList& args)
{
	createPrefabInternal(eEndCap, "patchCreateCaps");
}

void createBevel(const cmd::ArgumentList& args)
{
	createPrefabInternal(eBevel, "patchCreateBevel");
}

void createCone(const cmd::ArgumentList& args)
{
	createPrefabInternal(eCone, "patchCreateCone");
}

// Sanitise the integer to specify a valid patch dimension
// will return 0 if the input is invalid
std::size_t checkPatchDimension(int input)
{
	// Must be an odd number in [3..15]
	if (input < 3 || input > 15 || input % 2 == 0)
	{
		return 0;
	}

	// all good
	return static_cast<std::size_t>(input);
}

void createSimplePatch(const cmd::ArgumentList& args)
{
	std::size_t width = 0;
	std::size_t height = 0;
	bool removeSelectedBrush = false;

	if (args.empty() || args.size() > 3)
	{
		throw cmd::ExecutionFailure(_("Invalid number of arguments"));
	}
	if (args.size() == 1)
	{
		// Try to convert the arguments to actual integers and do the range checks
		width = height = checkPatchDimension(args[0].getInt());
	}
	else if (args.size() == 2)
	{
		width = checkPatchDimension(args[0].getInt());
		height = checkPatchDimension(args[1].getInt());
	}
	else if (args.size() == 3)
	{
		width = checkPatchDimension(args[0].getInt());
		height = checkPatchDimension(args[1].getInt());
		removeSelectedBrush = args[2].getBoolean();
	}

	// Only fire the dialog if no or invalid command arguments are given
	if (width != 0 && height != 0)
	{
		UndoableCommand undo("patchCreatePlane");

		// Retrieve the boundaries before any delete operation
		AABB bounds = getDefaultBoundsFromSelection();

		if (removeSelectedBrush)
		{
			// Delete the selection, the should be only one brush selected
			selection::algorithm::deleteSelection();
		}

		// Call the PatchConstruct routine (GtkRadiant legacy)
		constructPrefab(bounds,
			getSelectedShader(),
			ePlane, GlobalXYWndManager().getActiveViewType(),
			width, height);
	}
}

void createCaps(Patch& patch, const scene::INodePtr& parent, CapType type, const std::string& shader)
{
	if ((type == CapType::EndCap || type == CapType::InvertedEndCap) && patch.getWidth() != 5)
	{
		rError() << "cannot create end-cap - patch width != 5" << std::endl;

		throw cmd::ExecutionFailure(_("Cannot create end-cap, patch must have a width of 5."));

		return;
	}

	if ((type == CapType::Bevel || type == CapType::InvertedBevel) && patch.getWidth() != 3)
	{
		throw cmd::ExecutionFailure(_("Cannot create bevel-cap, patch must have a width of 3."));

		rError() << "cannot create bevel-cap - patch width != 3" << std::endl;
		return;

	}

	if (type == CapType::Cylinder && patch.getWidth() != 9)
	{
		throw cmd::ExecutionFailure(_("Cannot create cylinder-cap, patch must have a width of 9."));

		rError() << "cannot create cylinder-cap - patch width != 9" << std::endl;
		return;
	}

	assert(parent != NULL);

	{
		scene::INodePtr cap(GlobalPatchModule().createPatch(patch::PatchDefType::Def2));
		parent->addChildNode(cap);

		Patch* capPatch = Node_getPatch(cap);
		assert(capPatch != NULL);

		patch.MakeCap(capPatch, type, ROW, true);
		capPatch->setShader(shader);

		// greebo: Avoid creating "degenerate" patches (all vertices merged in one 3D point)
		if (!capPatch->isDegenerate())
		{
			Node_setSelected(cap, true);
		}
		else
		{
			parent->removeChildNode(cap);
			rWarning() << "Prevented insertion of degenerate patch." << std::endl;
		}
	}

	{
		scene::INodePtr cap(GlobalPatchModule().createPatch(patch::PatchDefType::Def2));
		parent->addChildNode(cap);

		Patch* capPatch = Node_getPatch(cap);
		assert(capPatch != NULL);

		patch.MakeCap(capPatch, type, ROW, false);
		capPatch->setShader(shader);

		// greebo: Avoid creating "degenerate" patches (all vertices merged in one 3D point)
		if (!capPatch->isDegenerate())
		{
			Node_setSelected(cap, true);
		}
		else
		{
			parent->removeChildNode(cap);
			rWarning() << "Prevented insertion of degenerate patch." << std::endl;
		}
	}
}

}

} // namespace
