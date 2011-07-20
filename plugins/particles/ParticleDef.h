#pragma once

#include "ParticleStage.h"

#include "iparticles.h"

#include "parser/DefTokeniser.h"
#include "string/string.h"
#include <vector>
#include <set>

namespace particles
{

/**
 * Representation of a single particle definition. Each definition is comprised
 * of a number of "stages", which must all be rendered in turn.
 */
class ParticleDef
: public IParticleDef
{
	// Name
	std::string _name;

	// The filename this particle has been defined in
	std::string _filename;

	// Depth hack
	float _depthHack;

	// Vector of stages
	typedef std::vector<ParticleStage> StageList;
	StageList _stages;

	typedef std::set<IParticleDef::Observer*> Observers;
	Observers _observers;

public:

	/**
	 * Construct a named ParticleDef.
	 */
	ParticleDef(const std::string& name)
	: _name(name)
	{ }

	/**
	 * Return the ParticleDef name.
	 */
	const std::string& getName() const
	{
		return _name;
	}

	const std::string& getFilename() const
	{
		return _filename;
	}

	void setFilename(const std::string& filename)
	{
		_filename = filename;
	}

	// Clears stage and depth hack information
	// Name and observers are NOT cleared
	void clear()
	{
		_depthHack = false;
		_stages.clear();
	}

	float getDepthHack() const
	{
		return _depthHack;
	}

	void setDepthHack(float value)
	{
		_depthHack = value;
	}

	std::size_t getNumStages() const
	{
		return _stages.size();
	}

	const IParticleStage& getParticleStage(std::size_t stageNum) const
	{
		return _stages[stageNum];
	}

	IParticleStage& getParticleStage(std::size_t stageNum)
	{
		return _stages[stageNum];
	}

	std::size_t addParticleStage() 
	{
		_stages.push_back(ParticleStage());

		return _stages.size() - 1;
	}

	void removeParticleStage(std::size_t index)
	{
		if (index < _stages.size())
		{
			_stages.erase(_stages.begin() + index);
		}
	}

	void swapParticleStages(std::size_t index, std::size_t index2)
	{
		if (index >= _stages.size() || index2 >= _stages.size() || index == index2)
		{
			return;
		}

		std::swap(_stages[index], _stages[index2]);

		// Notify any observers about this event
		for (Observers::const_iterator i = _observers.begin(); i != _observers.end();)
		{
			(*i++)->onParticleStageOrderChanged();
		}
	}

	void appendStage(const ParticleStage& stage)
	{
		_stages.push_back(stage);
	}

	void addObserver(IParticleDef::Observer* observer)
	{
		_observers.insert(observer);
	}

	void removeObserver(IParticleDef::Observer* observer)
	{
		_observers.erase(observer);
	}

	bool operator==(const IParticleDef& other) const 
	{
		// Compare depth hack flag
		if (getDepthHack() != other.getDepthHack()) return false;

		// Compare number of stages
		if (getNumStages() != other.getNumStages()) return false;

		// Compare each stage
		for (std::size_t i = 0; i < getNumStages(); ++i)
		{
			if (getParticleStage(i) != other.getParticleStage(i)) return false;
		}

		// All checks passed => equal
		return true;
	}

	bool operator!=(const IParticleDef& other) const
	{
		return !operator==(other);
	}

	void copyFrom(const IParticleDef& other)
	{
		setDepthHack(other.getDepthHack());

		_name = other.getName();
		_filename = other.getFilename();

		_stages.resize(other.getNumStages());

		for (std::size_t i = 0; i < _stages.size(); ++i)
		{
			_stages[i].copyFrom(other.getParticleStage(i));
		}
	}

	void parseFromTokens(parser::DefTokeniser& tok)
	{
		// Clear out the particle def (except the name) before parsing
		clear();

		// Any global keywords will come first, after which we get a series of
		// brace-delimited stages.
		std::string token = tok.nextToken();

		while (token != "}")
		{
			if (token == "depthHack")
			{
				setDepthHack(strToFloat(tok.nextToken()));
			}
			else if (token == "{")
			{
				// Construct/Parse the stage from the tokens
				ParticleStage stage(tok);

				// Append to the ParticleDef
				appendStage(stage);
			}

			// Get next token
			token = tok.nextToken();
		}

		// Notify any observers about this event
		for (Observers::const_iterator i = _observers.begin(); i != _observers.end();)
		{
			(*i++)->onParticleReload();
		}
	}
};
typedef boost::shared_ptr<ParticleDef> ParticleDefPtr;

}
