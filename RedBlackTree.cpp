#include <list>
#include <iostream>
using namespace std;

class RedBlackTree;

enum {
    RED,
    BLACK 
};


class RedBlackNode
{
    float    element;
    list<char*> *files;
    RedBlackNode *left;
    RedBlackNode *right;
    int           color;

    public:
    float getSimilarity();
    list<char*> * getFiles();
    RedBlackNode()
    {
    }

    RedBlackNode
    (   float elemValue,
            list<char*> *fileList,
        RedBlackNode *lt = NULL,
        RedBlackNode *rt = NULL,
        int c = 1)
    {
        element = elemValue;
        files = fileList;
        left = lt;
        right = rt;
        color = c;
    }

    ~RedBlackNode()
    {
        delete files;
    }

    friend class RedBlackTree;
};

float RedBlackNode::getSimilarity()
{
    return element;
}

list<char*>* RedBlackNode::getFiles()
{
    return files;
}

class RedBlackTree
{
    public:
    explicit RedBlackTree(float & negInf);
    RedBlackTree( RedBlackTree & rhs );
    ~RedBlackTree( );

    list<char*>* get(float x );
    bool isEmpty( );
    void printTree( );

    void makeEmpty( );
    void insert( float  x, list<char*> *list );
    
    RedBlackTree & operator=(RedBlackTree & rhs );
    RedBlackNode* next();
    int hasNext();
    void  createList();
    void  createList( RedBlackNode *t );


    private:
    RedBlackNode *header;
    float ITEM_NOT_FOUND;
    RedBlackNode *nullNode;

    RedBlackNode *current;
    RedBlackNode *parent;
    RedBlackNode *grand;
    RedBlackNode *great;
    list <RedBlackNode*> iter;
    list <RedBlackNode*>::iterator it;
    

    void reclaimMemory( RedBlackNode *t );
    void printTree( RedBlackNode *t );
    RedBlackNode * clone( RedBlackNode * t );

    void handleReorient(float & item );
    RedBlackNode * rotate( float & item, RedBlackNode *parent );
    void rotateWithLeftChild( RedBlackNode * & k2 );
    void rotateWithRightChild( RedBlackNode * & k1 );
};

RedBlackNode* RedBlackTree::next()
{
    RedBlackNode *node;
    if (it != iter.end())
        node = *it;
    it++;
    return node;
}
int RedBlackTree:: hasNext()
{
    if (it != iter.end())
        return 1;
    else
        return 0;
}
void RedBlackTree:: createList()
{
        if( header->right == nullNode )
            return;
        else
        {
            createList(header->right);
            it = iter.begin();
        }            
}
void RedBlackTree:: createList( RedBlackNode *t )
{
    if( t != t->left )
    {
        createList( t->left );
        iter.push_back(t);
        createList( t->right );
    }
}


/**
    * Construct the tree.
    * negInf is a value less than or equal to all others.
    * It is also used as ITEM_NOT_FOUND.
    */

RedBlackTree::RedBlackTree(float &negInf)
{
    nullNode    = new RedBlackNode;
    nullNode->left = nullNode->right = nullNode;
    header      = new RedBlackNode(negInf, NULL);
    header->left = header->right = nullNode;
}

/**
    * Copy constructor.
    */

RedBlackTree::RedBlackTree( RedBlackTree & rhs )
{
    nullNode    = new RedBlackNode;
    nullNode->left = nullNode->right = nullNode;
    header      = new RedBlackNode();
    header->left = header->right = nullNode;
    *this = rhs;
}

/**
    * Destroy the tree.
    */

RedBlackTree::~RedBlackTree( )
{
    makeEmpty( );
    delete nullNode;
    delete header;
}

/**
    * Insert item x into the tree. Does nothing if x already present.
    */

void RedBlackTree::insert(float  x , list<char*> *files)
{
    current = parent = grand = header;
    nullNode->element = x;

    while( current->element != x )
    {
        great = grand; grand = parent; parent = current;
        current = x < current->element ?  current->left : current->right;

        // Check if two red children; fix if so
        if( current->left->color == RED && current->right->color == RED )
                handleReorient( x );
    }

    // Insertion fails if already present
    if( current != nullNode )
        return;
    current = new RedBlackNode( x, files, nullNode, nullNode );

    // Attach to parent
    if( x < parent->element )
        parent->left = current;
    else
        parent->right = current;
    handleReorient( x );
}


/**
    * Find item x in the tree.
    * Return the matching item or ITEM_NOT_FOUND if not found.
    */
list<char*>*  RedBlackTree::get(float x )
{
    nullNode->element = x;
    RedBlackNode *curr = header->right;

    for(;;)
    {
        if( x < curr->element )
            curr = curr->left;
        else if( curr->element < x )
            curr = curr->right;
        else if( curr != nullNode )
            return curr->files;
        else
            return NULL;
    }
}

/**
    * Make the tree logically empty.
    */
void RedBlackTree::makeEmpty( )
{
    reclaimMemory( header->right );
    header->right = nullNode;
}

/**
    * Test if the tree is logically empty.
    * Return true if empty, false otherwise.
    */
bool RedBlackTree::isEmpty()
{
    return header->right == nullNode;
}

/**
    * Print the tree contents in sorted order.
    */
void RedBlackTree::printTree()
{
    if( header->right == nullNode )
        cout << "Empty tree" << endl;
    else
        printTree( header->right );
}


/**
    * Deep copy.
    */
RedBlackTree &RedBlackTree::operator=(RedBlackTree & rhs )
{
    if( this != &rhs )
    {
        makeEmpty( );
        header->right = clone( rhs.header->right );
    }

    return *this;
}

/**
    * Internal method to print a subtree t in sorted order.
    */
void RedBlackTree::printTree( RedBlackNode *t )
{
    if( t != t->left )
    {
        printTree( t->left );
        cout << t->element <<":";
        list<char*>::iterator it;
        for( it = t->files->begin(); it != t->files->end(); it++)
        {
            cout<<*it<<"  ";
        }           
        cout <<endl;
        printTree( t->right );
    }
}

/**
    * Internal method to clone subtree.
    */
RedBlackNode *
RedBlackTree::clone( RedBlackNode * t )
{
    if( t == t->left )
        return nullNode;
    else
        return new RedBlackNode( t->element, t->files, clone( t->left ),
                     clone( t->right ), t->color );
}


/**
    * Internal routine that is called during an insertion
    *     if a node has two red children. Performs flip
    *     and rotatons.
    * item is the item being inserted.
    */
void RedBlackTree::handleReorient(float & item )
{
    // Do the color flip
    current->color = RED;
    current->left->color = BLACK;
    current->right->color = BLACK;

    if( parent->color == RED )   // Have to rotate
    {
        grand->color = RED;
        if( item < grand->element != item < parent->element )
            parent = rotate( item, grand );  // Start dbl rotate
        current = rotate( item, great );
        current->color = BLACK;
    }
    header->right->color = BLACK; // Make root black
}

/**
    * Internal routine that performs a single or double rotation.
    * Because the result is attached to the parent, there are four cases.
    * Called by handleReorient.
    * item is the item in handleReorient.
    * parent is the parent of the root of the rotated subtree.
    * Return the root of the rotated subtree.
    */
RedBlackNode *RedBlackTree::rotate( float & item,
                        RedBlackNode *theParent )
{
    if( item < theParent->element )
    {
        item < theParent->left->element ?
            rotateWithLeftChild( theParent->left )  :  // LL
            rotateWithRightChild( theParent->left ) ;  // LR
        return theParent->left;
    }
    else
    {
        item < theParent->right->element ?
            rotateWithLeftChild( theParent->right ) :  // RL
            rotateWithRightChild( theParent->right );  // RR
        return theParent->right;
    }
}

/**
    * Rotate binary tree node with left child.
    */
void RedBlackTree::rotateWithLeftChild( RedBlackNode * & k2 )
{
    RedBlackNode *k1 = k2->left;
    k2->left = k1->right;
    k1->right = k2;
    k2 = k1;
}

/**
    * Rotate binary tree node with right child.
    */
void RedBlackTree::rotateWithRightChild( RedBlackNode * & k1 )
{
    RedBlackNode *k2 = k1->right;
    k1->right = k2->left;
    k2->left = k1;
    k1 = k2;
}


/**
    * Internal method to reclaim internal nodes
    * in subtree t.
    */
void RedBlackTree::reclaimMemory( RedBlackNode *t )
{
    if( t != t->left )
    {
        reclaimMemory( t->left );
        reclaimMemory( t->right );
        delete t;
    }
}

#if 0
int main( )
{    
    const float NEG_INF = -9999.0;
    float ITEM_NOT_FOUND = NEG_INF;
    RedBlackTree t( ITEM_NOT_FOUND );
    float nums[10] = {9.0,0.0,2.0,9.0,5.0,6.0,4.0,2.0,2.0,1.0};
    char filenames[10][6] = {"abcde", "bcdef", "cdefg","defgh", "efghi", "fghij", "ghijk", "hijkl", "ijklm", "jklmn"};
    int i;
    list <char*> *lst;
    list <char*> *lists;

    for( i = 0; i < 10;i++)
    {
        if ((lst = t.get(nums[i])) == NULL)
        {
            lists = new list<char*>;
            lists->push_back(filenames[i]);
            t.insert( nums[i], lists);
        }
        else
        {
            lst->push_back(filenames[i]);
        }
    }        

    t.printTree( );

    t.createList();
    while(t.hasNext())
    {
        RedBlackNode *node = t.next();
        cout << "  "<<node->getSimilarity()<<endl;
        list<char*> *files=node->getFiles();
        list<char*>::iterator it;
        for( it = files->begin(); it != files->end(); it++)
        {
            cout<<*it<<"  ";
        }           
        cout <<endl;
    }
}
#endif
