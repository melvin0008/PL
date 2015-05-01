#lang plai

(define-type ArithC
  [numC (n number?)]
  [plusC (l ArithC?)
         (r ArithC?)]
  [multC (l ArithC?)
         (r ArithC?)])


(define (interp e)
  (type-case ArithC e
    [num (n) n]
    [plus (l r) (+ (interp l)
                   (interp r))]
    [mul (l r) (* (interp l)
                  (interp r))]))
