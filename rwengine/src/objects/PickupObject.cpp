#include <objects/PickupObject.hpp>
#include <objects/CharacterObject.hpp>
#include <engine/GameWorld.hpp>

PickupObject::PickupObject(GameWorld *world, const glm::vec3 &position, int modelID)
	: GameObject(world, position, glm::quat(), nullptr),
	  _ghost(nullptr), _shape(nullptr), _enabled(false), collected(false), _modelID(modelID)
{
	btTransform tf;
	tf.setIdentity();
	tf.setOrigin(btVector3(position.x, position.y, position.z));

	_ghost = new btPairCachingGhostObject;
	_ghost->setUserPointer(this);
	_ghost->setWorldTransform(tf);
	_shape = new btSphereShape(0.5f);
	_ghost->setCollisionShape(_shape);
	_ghost->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT|btCollisionObject::CF_NO_CONTACT_RESPONSE);

	setEnabled(true);
}

PickupObject::~PickupObject()
{
	if(_ghost) {
		setEnabled(false);
		delete _ghost;
		delete _shape;
	}
}

void PickupObject::tick(float dt)
{
	if(! _enabled) {
		_enableTimer -= dt;
		if( _enableTimer <= 0.f ) {
			setEnabled(true);
			collected = false;
		}
	}

	if(_enabled) {
		// Sort out interactions with things that may or may not be players.
		btManifoldArray manifoldArray;
		btBroadphasePairArray& pairArray = _ghost->getOverlappingPairCache()->getOverlappingPairArray();
		int numPairs = pairArray.size();

		for (int i=0;i<numPairs;i++)
		{
			manifoldArray.clear();

			const btBroadphasePair& pair = pairArray[i];
			auto otherObject = static_cast<const btCollisionObject*>(
						pair.m_pProxy0->m_clientObject == _ghost ? pair.m_pProxy1->m_clientObject : pair.m_pProxy0->m_clientObject);
			if(otherObject->getUserPointer()) {
				GameObject* object = static_cast<GameObject*>(otherObject->getUserPointer());
				if(object->type() == Character) {
					CharacterObject* character = static_cast<CharacterObject*>(object);
					collected = onCharacterTouch(character);
					setEnabled( !collected );
					if( ! _enabled ) {
						_enableTimer = 60.f;
					}
				}
			}
		}

		auto tex = engine->gameData.textures[{"coronacircle",""}].texName;

		engine->renderer.addParticle({
										 position,
										 {0.f, 0.f, 1.f},
										 0.f,
										 GameRenderer::FXParticle::Camera,
										 engine->gameTime, dt,
										 tex,
										 {1.f, 1.f},
										 {}, {0.75f, 0.f, 0.f, 1.f} /** @todo configurable tint colour */
					});
	}
}

void PickupObject::setEnabled(bool enabled)
{
	if( ! _enabled && enabled ) {
		engine->dynamicsWorld->addCollisionObject(_ghost, btBroadphaseProxy::SensorTrigger);
	}
	else if( _enabled && ! enabled ) {
		engine->dynamicsWorld->removeCollisionObject(_ghost);
	}

	_enabled = enabled;
}
