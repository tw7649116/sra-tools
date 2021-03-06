echo "----- count how many alignments we have in a given slice -----"

# this example does the same as example6
# the only difference is that it uses a 'here document' in bash
TOOL="vdb-sql"
ACC="SRR341578"
ID="NC_011748.1"
START="123456"
END="133456"

$TOOL <<BEGIN_AND_END_OF_HERE_DOC
create virtual table VDB using vdb( $ACC, table=PRIM );
select count() from VDB where REF_SEQ_ID='$ID' and REF_POS >= $START and REF_POS <= $END;
BEGIN_AND_END_OF_HERE_DOC
