#include "UCC.h"



enum State
{
	// DONE: Add some states
	ST_WAIT_RESOURCE_REQUEST,
	ST_NEGOTIATION_END,
	ST_WAIT_NEGOTIATION
};

UCC::UCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId) :
	Agent(node),
	_contributedItemId(contributedItemId),
	_constraintItemId(constraintItemId),
	_negotiationAgreement(false)
{
	setState(ST_WAIT_RESOURCE_REQUEST);
}

UCC::~UCC()
{
}

void UCC::update()
{
	// Nothing to do
}

void UCC::finalize()
{
	finish();
}

void UCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	PacketType packetType = packetHeader.packetType;
	// DONE Receive requests and send back responses...

	if(state() == ST_WAIT_RESOURCE_REQUEST && packetType == PacketType::ItemRequestToUCC)
	{
		if(_contributedItemId == NULL_ITEM_ID)
		{
			//Offer resource
			OutputMemoryStream outStream;
			PacketHeader outPckHead;
			outPckHead.packetType = PacketType::ResourceOfferToUCP;
			outPckHead.dstAgentId = packetHeader.srcAgentId;
			outPckHead.Write(outStream);
			socket->SendPacket(outStream.GetBufferPtr(), outStream.GetSize());

			setState(ST_NEGOTIATION_END);
			_negotiationAgreement = true;
		}
		else
		{
			// Send constrain
			OutputMemoryStream outStream;
			PacketHeader outPacketHead;
			outPacketHead.packetType = PacketType::ItemConstrain;
			outPacketHead.dstAgentId = packetHeader.srcAgentId;
			outPacketHead.Write(outStream);

			PacketItemConstrainToUCP outData;
			outData.itemId = _contributedItemId;
			outData.Write(outStream);

			socket->SendPacket(outStream.GetBufferPtr(), outStream.GetSize());

			setState(ST_NEGOTIATION_END);
		}
	}
	else if(state() == ST_WAIT_NEGOTIATION && packetType == PacketType::ResourceNegotiationEnd)
	{
		PacketResourceNegotiationEnd pckData;
		pckData.Read(stream);
		_negotiationAgreement = pckData.agreement;

		setState(ST_NEGOTIATION_END);
	}
}

bool UCC::negotiationFinished() const {
	// DONE
	return state() == ST_NEGOTIATION_END;
}

bool UCC::negotiationAgreement() const {
	// DONE
	return _negotiationAgreement;
}
