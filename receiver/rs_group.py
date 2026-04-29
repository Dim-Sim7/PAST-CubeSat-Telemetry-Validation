


"""_summary_
This class represents one reed-solomon block, that is [DATA SHARD + PARITY SHARD]
On the first instance of a block packet, in the data, the size of the total data being sent is encoded
A dict of objects of this class is held in the receiver self._rs_groups: Dict[int, RSGroup] = {}
Also, when 
When packets with the same block id are received, the dict is searched with the id as the key,
shard are accumulated using the feed method here

In receiver, once all shards are collected
"""
class RSGroup:
    pass