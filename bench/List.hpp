/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */

#ifndef LIST_HPP__
#define LIST_HPP__

#define XBEGIN asm(" movl $1028, %ecx\n\t"  "xchg %rcx, %rcx")
#define XEND asm(" movl $1029, %ecx\n\t"  "xchg %rcx, %rcx")

#undef TM_READ
#define TM_READ(x) (x)
// We construct other data structures from the List. In order to do their
// sanity checks correctly, we might need to pass in a validation function of
// this type
typedef bool (*verifier)(uint32_t, uint32_t);

// Set of LLNodes represented as a linked list in sorted order
class List
{

  // Node in a List
  struct Node
  {
      int m_val;
      Node* m_next;

      // ctors
      Node(int val = -1) : m_val(val), m_next() { }

      Node(int val, Node* next) : m_val(val), m_next(next) { }
  };

  public:

    Node* sentinel;

    List();

    // true iff val is in the data structure
    TM_CALLABLE
    bool lookup(int val TM_ARG) const;

    TM_CALLABLE
    void update( TM_ARG_ALONE) ;


    // standard IntSet methods
    TM_CALLABLE
    bool insert(int val TM_ARG);

    // remove a node if its value = val
    TM_CALLABLE
    bool remove(int val TM_ARG);

    // make sure the list is in sorted order
    bool isSane() const;

    // make sure the list is in sorted order and for each node x,
    // v(x, verifier_param) is true
    bool extendedSanityCheck(verifier v, uint32_t param) const;

    // find max and min
    TM_CALLABLE
    int findmax(TM_ARG_ALONE) const;

    TM_CALLABLE
    int findmin(TM_ARG_ALONE) const;

    // overwrite all elements up to val
    TM_CALLABLE
    void overwrite(int val TM_ARG);
};


// constructor just makes a sentinel for the data structure
//List::List() : sentinel(new Node()) { }
List::List(){
  sentinel = (Node*)sitemalloc(sizeof(Node));
  new (sentinel) Node();
}

// simple sanity check: make sure all elements of the list are in sorted order
bool List::isSane(void) const
{

  bool correct = true;
  const Node* prev(sentinel);
  const Node* curr((prev->m_next));
  int elems = 0;
  while (curr != NULL) {


    if ((prev->m_val) >= (curr->m_val))
      correct = false;
    prev = curr;
    curr = curr->m_next;
    elems++;
  }
  std::cout << "Elements in the List at End: " << elems << std::endl;
  const Node* prev2(sentinel);
  const Node* curr2((prev2->m_next));

  if(!correct){
    while (curr2 != NULL) {
      
      prev2 = curr2;
      curr2 = curr2->m_next;
      
    }

    return false;
  }

  return true;
}

// extended sanity check, does the same as the above method, but also calls v()
// on every item in the list
bool List::extendedSanityCheck(verifier v, uint32_t v_param) const
{
    const Node* prev(sentinel);
    const Node* curr((prev->m_next));
    while (curr != NULL) {
        if (!v((curr->m_val), v_param) || ((prev->m_val) >= (curr->m_val)))
            return false;
        prev = curr;
        curr = prev->m_next;
    }
    return true;
}

void List::update(TM_ARG_ALONE){
  //   Node* prev(sentinel);
  Node* prev(sentinel);
  Node* curr((prev->m_next));
  std::cout << "Print curr next " << prev->m_next << " sentinel " << prev << " tid " << pthread_self()<< std::endl;

  while (curr != NULL) {

    std::cout << "Print curr next " << curr->m_next << " val " << curr->m_val << " tid " << pthread_self()<< std::endl;
   
    prev = curr;
    curr = curr->m_next;
    //TM_WRITE(prev->m_next, TM_READ(prev->m_next));
  }
  //  TM_WRITE(prev->m_next, TM_READ(prev->m_next));
}

// insert method; find the right place in the list, add val so that it is in
// sorted order; if val is already in the list, exit without inserting
TM_CALLABLE
bool List::insert(int val TM_ARG)
{
    // traverse the list to find the insertion point
    const Node* prev(sentinel);
    const Node* curr(TM_READ(prev->m_next));
    //std::cout << "insert" << std::endl;
    while (curr != NULL) {
      //      std::cout << curr->m_val << " curr mval " << std::endl;
        if (TM_READ(curr->m_val) >= val)
            break;
        prev = curr;
	curr = TM_READ(prev->m_next);
    }

    // now insert new_node between prev and curr
    if (!curr || (TM_READ(curr->m_val) > val)) {
        Node* insert_point = const_cast<Node*>(prev);

        // create the new node
        Node* i = (Node*)TM_ALLOC(sizeof(Node));
        //i->m_val = val;
        //i->m_next = const_cast<Node*>(curr);
        TM_WRITE(i->m_val, val);
        TM_WRITE(i->m_next, const_cast<Node*>(curr));
        TM_WRITE(insert_point->m_next, i);
	return true;
    }
    return false;
}


// search function
TM_CALLABLE
bool List::lookup(int val TM_ARG) const
{
    bool found = false;
    const Node* curr(sentinel);
    curr = TM_READ(curr->m_next);

    while (curr != NULL) {
        if (TM_READ(curr->m_val) >= val)
            break;
        curr = TM_READ(curr->m_next);
    }
    found = ((curr != NULL) && (TM_READ(curr->m_val) == val));
    return found;
}

// findmax function
TM_CALLABLE
int List::findmax(TM_ARG_ALONE) const
{
    int max = -1;
    const Node* curr(sentinel);
    while (curr != NULL) {
        max = TM_READ(curr->m_val);
        curr = TM_READ(curr->m_next);
    }
    return max;
}

// findmin function
TM_CALLABLE
int List::findmin(TM_ARG_ALONE) const
{
    int min = -1;
    const Node* curr(sentinel);
    curr = TM_READ(curr->m_next);
    if (curr != NULL)
        min = TM_READ(curr->m_val);
    return min;
}

// remove a node if its value == val
TM_CALLABLE
bool List::remove(int val TM_ARG)
{
  //  std::cout << "remove" << sentinel << std::endl;
      // find the node whose val matches the request
    const Node* prev(sentinel);
    const Node* curr(TM_READ(prev->m_next));
    while (curr != NULL) {
        // if we find the node, disconnect it and end the search
        if (TM_READ(curr->m_val) == val) {

            Node* mod_point = const_cast<Node*>(prev);
            TM_WRITE(mod_point->m_next, TM_READ(curr->m_next));
	    TM_WRITE(((Node*)curr)->m_next, (Node*)NULL); //dummy write
            // delete curr...
	    TM_FREE(const_cast<Node*>(curr));
	    return true;
            //break;
        }
        else if (TM_READ(curr->m_val) > val) {
	  //	  TM_WRITE(((Node*)curr)->m_val, ((Node*)curr)->m_val);
	  // this means the search failed
	  return false;
	  // break;
        }
        prev = curr;
	curr = TM_READ(prev->m_next);
    }
    //    printf("remove\n");
    return false;
}

// search function
TM_CALLABLE
void List::overwrite(int val TM_ARG)
{
    const Node* curr(sentinel);
    curr = TM_READ(curr->m_next);

    while (curr != NULL) {
        if (TM_READ(curr->m_val) >= val)
            break;
        Node* wcurr = const_cast<Node*>(curr);
        TM_WRITE(wcurr->m_val, TM_READ(wcurr->m_val));
        curr = TM_READ(wcurr->m_next);
    }
}

#endif // LIST_HPP__


